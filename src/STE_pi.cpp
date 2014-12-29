/******************************************************************************
 * $Id: STE_pi.cpp, v0.2 2011/05/23 SethDart Exp $
 *
 * Project:  OpenCPN
 * Purpose:  STE Plugin
 * Author:   Jean-Eudes Onfray
 *
 ***************************************************************************
 *   Copyright (C) 2011 by Jean-Eudes Onfray   *
 *   $EMAIL$   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************
 */


#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
  #include "wx/wx.h"
#endif 


//#include <math.h>
#include <fstream>
#include <wx/fileconf.h>
#include <wx/textfile.h>
//#include <wx/listctrl.h>
#include <list>
#include <wx/tokenzr.h>
#include "STE_pi.h"
#include "icons.h"

using namespace std;

bool  shown_dc_message;
bool tool_bar_set = false;
double pos_lat, pos_lon;
long start_record = 1, end_record = 1, record_count;
bool start_end_change = true;
long wstyle = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER;

int FileType = 0; // trt file
double track_interval;
int track_int_sel;

wxTextFile   m_istream;
STE_Point    NMEA_out_point;

#include <wx/listimpl.cpp>
WX_DEFINE_LIST(STE_PointList);
//STE_Analysis m_Analysis_Track;
STE_PointList *m_pSTE_PointList = NULL;
PlugIn_Track *m_pTrack = NULL;


double sum_rws, count_r, sum_rwd;
double sum_tws, count_t, sum_twd;
double var;
wxString UTS_time, UTS_date;

struct wind{
    double TWA;
    double RWA;
    double TWS;
    double RWS;
    double TWD;
} Wind;


struct boat{
    double SOG;
    double COG;
    double STW;
    double HDG;
    double HDGM;
} Boat;

wxString          m_ifilename;
wxString          m_ofilename;


enum {                                      // process ID's
    wxEVT_START_SLIDER_UPDATED = 10000,
    wxEVT_END_SLIDER_UPDATED,
    ID_OK,
    ID_POINT_INTERVAL,
    ID_FILETYPE
};

// the class factories, used to create and destroy instances of the PlugIn

extern "C" DECL_EXP opencpn_plugin* create_pi(void *ppimgr)
{
    return new STE_pi(ppimgr);
}

extern "C" DECL_EXP void destroy_pi(opencpn_plugin* p)
{
    delete p;
}

//---------------------------------------------------------------------------------------------------------
//
//    STE PlugIn Implementation
//
//---------------------------------------------------------------------------------------------------------

STE_pi::STE_pi(void *ppimgr)
      :opencpn_plugin_111(ppimgr)
{
      // Create the PlugIn icons
      initialize_images();
}

STE_pi::~STE_pi(void)
{
    delete _img_STE_play;
}

int STE_pi::Init(void)
{
      AddLocaleCatalog( _T("opencpn-STE_pi") );

      //    Get a pointer to the opencpn configuration object
      m_pconfig = GetOCPNConfigObject();
      LoadConfig();
      m_pauimgr = GetFrameAuiManager();
      m_parent_window = GetOCPNCanvasWindow();
      m_pSTE_Control = NULL;

      var = -14.0;      // TODO make this an input

      //    And load the configuration items
//      LoadConfig();

      m_PlugIn_STE = InsertPlugInTool(_T(""), _img_STE_play, _img_STE_play, wxITEM_CHECK,
            _("STE"), _T(""), NULL, STE_TOOL_POSITION, 0, this);

      return (
           WANTS_TOOLBAR_CALLBACK    |
           INSTALLS_TOOLBAR_TOOL     |
//           WANTS_CONFIG              |
           WANTS_OPENGL_OVERLAY_CALLBACK |
           WANTS_OVERLAY_CALLBACK     |
//           WANTS_CURSOR_LATLON        |           
           WANTS_PREFERENCES
            );
}

bool STE_pi::DeInit(void)
{
    SaveConfig();
    if ( m_pSTE_Control )
        {		
            m_pauimgr->DetachPane( m_pSTE_Control );
            m_pSTE_Control->Close();
            m_pSTE_Control->Destroy();
            m_pSTE_Control = NULL;
        }

     if (m_pSTE_PointList)
         if (!m_pSTE_PointList->empty())
        {
            m_pSTE_PointList->DeleteContents(true);
            m_pSTE_PointList->Clear();
        }

     if (m_pTrack){
         if (!m_pTrack->pWaypointList->empty())
        {  
            m_pTrack->pWaypointList->DeleteContents(true);
        }

        DeletePlugInTrack(m_pTrack->m_GUID);
     }

    return true;
}

int STE_pi::GetAPIVersionMajor()
{
      return MY_API_VERSION_MAJOR;
}

int STE_pi::GetAPIVersionMinor()
{
      return MY_API_VERSION_MINOR;
}

int STE_pi::GetPlugInVersionMajor()
{
      return PLUGIN_VERSION_MAJOR;
}

int STE_pi::GetPlugInVersionMinor()
{
      return PLUGIN_VERSION_MINOR;
}

wxBitmap *STE_pi::GetPlugInBitmap()
{
      return _img_STE_pi;
}

wxString STE_pi::GetCommonName()
{
      return _("STE-Editor");
}


wxString STE_pi::GetShortDescription()
{
      return _("Sea Track plugin for OpenCPN");
}

wxString STE_pi::GetLongDescription()
{
      return _("Sea Track Edit (STE) plugin for OpenCPN\n\
Uses Sea Trace (trt) files.");

}
//*******************************************************************************
// ToolBar Actions
//*******************************************************************************

int STE_pi::GetToolbarToolCount(void)
{
      return 1;
}

void STE_pi::OnToolbarToolCallback(int id)
{
      if ( id == m_PlugIn_STE )
      {
          if (!tool_bar_set)
            {
               SetToolbarItemState( m_PlugIn_STE, true );
               tool_bar_set = true;

               if (  m_istream.IsOpened() ) // File open already so close it down
               {
                     m_istream.Close();
                     if ( m_pSTE_Control )
                      {		
                            m_pauimgr->DetachPane( m_pSTE_Control );
                            m_pSTE_Control->Close();
                            m_pSTE_Control->Destroy();
                            m_pSTE_Control = NULL;
                      }
                     SetToolbarItemState( m_PlugIn_STE, false );
                     tool_bar_set = false;
                     return;
               }

                wxString message = _("Select trt file");
                wxString filetypext = _("*.trt");
                if (FileType == 1) {
                    message = _("Select txt file");
                    filetypext = _("*.txt");
                }

                wxFileDialog fdlg( m_parent_window, message , _T(""), m_ifilename, filetypext, wxFD_OPEN|wxFD_FILE_MUST_EXIST );
                if ( fdlg.ShowModal() != wxID_OK)
                    {
                        SetToolbarItemState( m_PlugIn_STE, false );
                        tool_bar_set = false;
                        return;
                    }

                m_ifilename.Clear();
                m_ifilename = fdlg.GetPath();

        //********************** Create trt file from NMEA message file (txt) ************************

                if (FileType == 1)
                {
                    if(!Loadtxt_trt())                      // convert file and rename m_ifilename
                    {
                        SetToolbarItemState( m_PlugIn_STE, false );
                        tool_bar_set = false;
                    }
                }
                else
                {
                    m_istream.Open( m_ifilename );
                    record_count = m_istream.GetLineCount();

                    if (!m_pSTE_Control )
                    {
                        m_pSTE_Control = new STE_Control( m_parent_window, wxID_ANY, this, 1, 100, m_istream.GetLineCount(), m_ifilename);                       
                        wxAuiPaneInfo pane = wxAuiPaneInfo().Name(_T("STE")).Caption(_("STE Control")).CaptionVisible(true).Float().FloatingPosition(50,100).Dockable(false).Fixed().CloseButton(false).Show(true);
                        m_pauimgr->AddPane( m_pSTE_Control, pane );
                        m_pauimgr->Update();
                    }
                  
                    m_pTrack = new PlugIn_Track;
                    m_pSTE_PointList = new STE_PointList;
        
                    SetStart(0);
                    SetEnd(100);
                    Load_track();
                }
            }
//*********** De-select Plug-in ******************************
            else
            {
               SetToolbarItemState( m_PlugIn_STE, false );
               tool_bar_set = false;

               if ( m_pSTE_Control )
                      {		
                            m_pauimgr->DetachPane( m_pSTE_Control );
                            m_pSTE_Control->Close();
                            m_pSTE_Control->Destroy();
                            m_pSTE_Control = NULL;
                      }
               if (m_pSTE_PointList)
                   if (!m_pSTE_PointList->empty())
                    {
                        m_pSTE_PointList->DeleteContents(true);
                        m_pSTE_PointList->Clear();
                    }

                if (m_pTrack)
                    if (!m_pTrack->pWaypointList->empty())
                    {  
                        m_pTrack->pWaypointList->Clear();
                        DeletePlugInTrack(m_pTrack->m_GUID);
                    }
            }
      }
}

//*********************************************************************************
// Display Preferences Dialog
//*********************************************************************************

void STE_pi::ShowPreferencesDialog(wxWindow* parent)
{
    wxDialog *dialog = new wxDialog( parent, wxID_ANY, _("STE Input File type (txt or TRT)"),
        wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE);

    int border_size = 4;

    wxBoxSizer* Display_Preferencs_panel = new wxBoxSizer(wxVERTICAL);
	dialog->SetSizer(Display_Preferencs_panel);

    wxStaticBox* StaticBoxSizer = new wxStaticBox(dialog, wxID_ANY, _("STE Configuration"));
    wxStaticBoxSizer* BoxSizerSTE = new wxStaticBoxSizer(StaticBoxSizer, wxVERTICAL);
    Display_Preferencs_panel->Add(BoxSizerSTE, 0, wxGROW|wxALL, border_size);

    wxString FileTypeStrings[] = {
        _("SeaTrace (trt) files"),
        _("VDR (txt) files"),
    };

    pFileType = new wxRadioBox(dialog, ID_FILETYPE, _("File Type"),
                                    wxDefaultPosition, wxDefaultSize,
                                    2, FileTypeStrings, 1, wxRA_SPECIFY_COLS);

    BoxSizerSTE->Add(pFileType, 0, wxALL | wxEXPAND, 2);
    
    pFileType->SetSelection(FileType);

    wxString track_int[] = {
        _("Minimum leg size"),
        _("500 feet (.1 mile)"),
        _("50 feet (.01 mile)")
    };

    pTrackInterval = new wxRadioBox(dialog, ID_POINT_INTERVAL, _("Track Minimum Interval"),
                                    wxDefaultPosition, wxDefaultSize,
                                    3, track_int, 1, wxRA_SPECIFY_COLS);

    BoxSizerSTE->Add(pTrackInterval, 0, wxALL | wxEXPAND, 2);
	pTrackInterval->SetSelection(track_int_sel);

    wxStdDialogButtonSizer* DialogButtonSizer = dialog->CreateStdDialogButtonSizer(wxOK|wxCANCEL);
    Display_Preferencs_panel->Add(DialogButtonSizer, 0, wxALIGN_RIGHT|wxALL, 5);
	
	dialog->Fit();

//********************** Input Data selections by modal ********************
    if(dialog->ShowModal() == wxID_OK)  // Use this instead of Event driven routines.
      {
		FileType = pFileType->GetSelection();

		track_int_sel = pTrackInterval->GetSelection();

        switch(track_int_sel)
	    {
		    case 0:
                track_interval = 0;
                break;
            case 1:
                track_interval = 0.1;
                break;
            case 2:
                track_interval = 0.01;
                break;
	    }
        SaveConfig();
	}
}

/*
void STE_pi::SetColorScheme(PI_ColorScheme cs)
{
      if ( m_pSTE_Control )
      {
            m_pSTE_Control->SetColorScheme( cs );
      }
}
*/
bool STE_pi::LoadConfig(void)
{
      wxFileConfig *pConf = (wxFileConfig *)m_pconfig;

      if(pConf)
      {
            pConf->SetPath( _T("/PlugIns/STE") );
            pConf->Read( _T("TrackInterval"), &track_interval, .1 );

            return true;
      }
      else
            return false;
}

bool STE_pi::SaveConfig(void)
{
      wxFileConfig *pConf = (wxFileConfig *)m_pconfig;

      if(pConf)
      {
            pConf->SetPath( _T("/PlugIns/STE") );
            pConf->Write( _T("TrackInterval"), track_interval);

            return true;
      }
      else
            return false;
}


//***************************************************************
//      Local functions
//***************************************************************
bool STE_pi::Loadtxt_trt( void )
{
    bool trt_file_ok = false;
    wxTextFile instream;
    wxString outfilename;
    wxFile outstream;

    outfilename.Clear();
    outfilename = m_ifilename;

    // change extension from .txt to .trt                    
    outfilename.RemoveLast(2);
    outfilename = outfilename + _T("rt");
    
    instream.Open( m_ifilename );
    double file_size = (double)instream.GetLineCount();
    if (file_size > 10000)
    {
        wxString m_string = m_string.Format(_(" This file has %5.0f records. Continue?"), file_size); // instream.GetLineCount()));
        int do_file = wxMessageBox(m_string, _(""), wxYES_NO);
        if(do_file == wxNO) return false;
    }

    outstream.Open( outfilename, wxFile::write );
    if (outstream.IsOpened()) {
        wxString sentence ;       
        long end_infile;

        end_infile= instream.GetLineCount();
        for (long in_record = 1; in_record < end_infile; ++in_record) {
            sentence = instream.GetLine(in_record);

            if( make_trt_line (sentence)){            
                wxString trt_line = build_trt_string();
                outstream.Write(trt_line);
                NMEA_out_point.Clear();             // Clear Output string template
                clear_variables();

            }            
        }       
        outstream.Close();
        m_ifilename = outfilename;
        trt_file_ok = true;

        int answer = wxMessageBox(_("File converted. Another?"), _( ""),wxYES_NO);
        if (answer == wxNO)
        {
            FileType = 0;
        }
    }
    else{
        wxLogError(_T("Cannot save STE NMEA data in file '%s'."), outfilename);
        trt_file_ok = false;
    }
    instream.Close();
    return trt_file_ok; 
}

bool STE_pi::make_trt_line(wxString sentence)   // test and fill in true wind data
{
    bool make_OK = false;
    wxString m_string;
    if(NMEA_parse(sentence))
    {
        if (Boat.COG < 0 && Boat.HDGM >= 0.0)                        // Boat.COG not given but Boat.HDGM is
        {
                Boat.COG = Boat.HDGM + var ;
                NMEA_out_point.COGT = m_string.Format(_T("%3.1f"), Boat.COG);
        }

        if (Boat.SOG > 2.0 && Boat.COG >= 0.0 && Wind.TWS <= 0.0){        
            if (Wind.RWA >= 0.0 && Wind.RWS > 2.0)
            {
                Wind.TWS = TWS(Wind.RWS,Wind.RWA,Boat.SOG);
                Wind.TWA = TWA(Wind.RWS,Wind.RWA,Boat.SOG);

                NMEA_out_point.TrueWind = m_string.Format(_T("%5.3f"), Wind.TWA);
                NMEA_out_point.TWSpd = m_string.Format(_T("%3.1f"), Wind.TWS);
            }

            if (Wind.TWA >= 0.0 && Wind.TWS > 2.0 && Wind.RWS <= 0.0)
            {
                Wind.RWS = RWS(Wind.TWS,Wind.TWA,Boat.SOG);
                Wind.RWA = RWA(Wind.TWS,Wind.TWA,Boat.SOG);

                NMEA_out_point.RelWind = m_string.Format(_T("%5.3f"), Wind.RWA);
                NMEA_out_point.RWSpd = m_string.Format(_T("%3.1f"), Wind.RWS);
            }

            Wind.TWD = Wind.TWA + Boat.COG;
            if (Wind.TWD > 360) Wind.TWD = Wind.TWD - 360;
            NMEA_out_point.CrseWind =  m_string.Format(_T("%3.1f"), Wind.TWD);
        }

        if(!NMEA_out_point.UTS.IsEmpty()){
           if(!NMEA_out_point.Lat.IsEmpty() && !NMEA_out_point.Lon.IsEmpty()){
                if(!NMEA_out_point.SOG.IsEmpty() && !NMEA_out_point.COGT.IsEmpty()){
                    if(!NMEA_out_point.RelWind.IsEmpty() && !NMEA_out_point.RWSpd.IsEmpty())
                        make_OK = true;
                }
           }
        }           
    } 
    return make_OK;
}

bool STE_pi::NMEA_parse( wxString sentence)
{
    wxString m_string, m_yr;
    bool string_parsed = false;
    double lat, lon;

    m_NMEA0183 << sentence;
    if( m_NMEA0183.Parse()  == true)
    {
        if( m_NMEA0183.LastSentenceIDParsed == _T("GLL") )
        {
            float llt = m_NMEA0183.Gll.Position.Latitude.Latitude;
            int lat_deg_int = (int) ( llt / 100 );
            float lat_deg = lat_deg_int;
            float lat_min = llt - ( lat_deg * 100 );
            lat = lat_deg + ( lat_min / 60. );
            if( m_NMEA0183.Gll.Position.Latitude.Northing == South ) lat = -lat;
            NMEA_out_point.Lat = m_string.Format(_T("%06.5f"), lat);

            float lln = m_NMEA0183.Gll.Position.Longitude.Longitude;
            int lon_deg_int = (int) ( lln / 100 );
            float lon_deg = lon_deg_int;
            float lon_min = lln - ( lon_deg * 100 );
            lon = lon_deg + ( lon_min / 60. );
            if( m_NMEA0183.Gll.Position.Longitude.Easting == West ) lon = -lon;
            NMEA_out_point.Lon = m_string.Format(_T("%06.5f"), lon);

            wxString time = m_NMEA0183.Gll.UTCTime;
                    double value;
                    if(time.ToDouble(&value))
                    {
                        long hr = long( value / 10000);
                        long min = long(value / 100) - (hr * 100);
                        long sec = long(value) - (hr * 10000) - (min * 100);

                        UTS_time = wxString::Format(wxT("%02d:%02d:%02d NF"),hr,min,sec);
                    }
            string_parsed = true;
        }

        else if ( m_NMEA0183.LastSentenceIDParsed == _T("HDG") )
        {
            if( !wxIsNaN( m_NMEA0183.Hdg.MagneticVariationDegrees ) )
            {
                var =  m_NMEA0183.Hdg.MagneticVariationDegrees;
                if( m_NMEA0183.Hdg.MagneticVariationDirection == West )
                    var = - var;
            }
            NMEA_out_point.Variation = m_string.Format(_T("%5.3f"), var);

            if( !wxIsNaN( m_NMEA0183.Hdg.MagneticSensorHeadingDegrees ) )
            {
                Boat.HDG = m_NMEA0183.Hdg.MagneticSensorHeadingDegrees + var;
                if (Boat.HDG > 360) Boat.HDG = Boat.HDG - 360;
                NMEA_out_point.BtHDG = m_string.Format(_T("%5.3f"), Boat.HDG);
            }
            string_parsed = true;
        }

        else if( m_NMEA0183.LastSentenceIDParsed == _T("HDM") )
        {
            if( !wxIsNaN( m_NMEA0183.Hdm.DegreesMagnetic) )
            {
                Boat.HDG = m_NMEA0183.Hdm.DegreesMagnetic + var;               
                NMEA_out_point.BtHDG = m_string.Format(_T("%5.3f"), Boat.HDG);
            }
            string_parsed = true;
        }

        else if( m_NMEA0183.LastSentenceIDParsed == _T("HDT") )
        {
            if( !wxIsNaN( m_NMEA0183.Hdt.DegreesTrue) )
            {
                Boat.HDG = m_NMEA0183.Hdt.DegreesTrue;
                NMEA_out_point.BtHDG = m_string.Format(_T("%5.3f"), Boat.HDG);
            }
        }

        else if( m_NMEA0183.LastSentenceIDParsed == _T("VTG") )
        {           
            if( m_NMEA0183.Vtg.SpeedKnots < 999. )
            {
            }

            if( m_NMEA0183.Vtg.TrackDegreesTrue < 999. )
            {
                        
            }
            string_parsed = true;
        }

        else if( m_NMEA0183.LastSentenceIDParsed == _T("MWV") )
        {
            double wind_speed;
            wind_speed = m_NMEA0183.Mwv.WindSpeed;

            if(  wind_speed < 100. )
            {               
                if (m_NMEA0183.Mwv.WindSpeedUnits == _T("K"))
                {
                    wind_speed = wind_speed / 1.852;
                }
                else if (m_NMEA0183.Mwv.WindSpeedUnits == _T("M"))
                {
                    wind_speed = wind_speed / 1.1515;
                }

                if(m_NMEA0183.Mwv.Reference == _T("R"))
                {
                    sum_rwd = sum_rwd + m_NMEA0183.Mwv.WindAngle;
                    count_r = count_r + 1;
                    sum_rws =  sum_rws + wind_speed;

                    Wind.RWA = sum_rwd/count_r;
                    Wind.RWS = sum_rws/count_r;
                    NMEA_out_point.RelWind = m_string.Format(_T("%5.3f"), Wind.RWA);
                    NMEA_out_point.RWSpd = m_string.Format(_T("%3.1f"), Wind.RWS);
                }
                else
                {
                    sum_twd = sum_twd + m_NMEA0183.Mwv.WindAngle;
                    count_t = count_t + 1;
                    sum_tws = sum_tws + wind_speed;

                    Wind.TWA = sum_twd/count_t;
                    Wind.TWS = sum_tws/count_t;
                    NMEA_out_point.TrueWind = m_string.Format(_T("%5.3f"), Wind.TWA);
                    NMEA_out_point.TWSpd = m_string.Format(_T("%3.1f"), Wind.TWS);
                }
            }
            string_parsed = true;
        }
 
        else if( m_NMEA0183.LastSentenceIDParsed == _T("VHW") )
        {
            if( m_NMEA0183.Vhw.Knots < 999. )
            {
            }
            string_parsed = true;
        }
        else if( m_NMEA0183.LastSentenceIDParsed == _T("RMC") )
        {
                if( m_NMEA0183.Rmc.IsDataValid == NTrue )
                {
                    float llt = m_NMEA0183.Rmc.Position.Latitude.Latitude;
                    int lat_deg_int = (int) ( llt / 100 );
                    float lat_deg = lat_deg_int;
                    float lat_min = llt - ( lat_deg * 100 );
                    lat = lat_deg + ( lat_min / 60. );
                    if( m_NMEA0183.Rmc.Position.Latitude.Northing == South ) lat = -lat;                        
                    NMEA_out_point.Lat = m_string.Format(_T("%06.5f"), lat);

                    float lln = m_NMEA0183.Rmc.Position.Longitude.Longitude;
                    int lon_deg_int = (int) ( lln / 100 );
                    float lon_deg = lon_deg_int;
                    float lon_min = lln - ( lon_deg * 100 );
                    lon = lon_deg + ( lon_min / 60. );
                    if( m_NMEA0183.Rmc.Position.Longitude.Easting == West ) lon = -lon;
                    NMEA_out_point.Lon = m_string.Format(_T("%06.5f"), lon);

                    if( m_NMEA0183.Rmc.SpeedOverGroundKnots > .2 )
                    {
                        Boat.SOG = m_NMEA0183.Rmc.SpeedOverGroundKnots;
                        NMEA_out_point.SOG = m_string.Format(_T("%3.1f"), Boat.SOG);
                    }

                    if( m_NMEA0183.Rmc.TrackMadeGoodDegreesTrue < 360. )
                    {
                        Boat.COG = m_NMEA0183.Rmc.TrackMadeGoodDegreesTrue;
                        NMEA_out_point.COGT = m_string.Format(_T("%5.4f"), Boat.COG);
                    }
                   
                    if( !wxIsNaN(m_NMEA0183.Rmc.MagneticVariation) )
                    {                                            
                        var = m_NMEA0183.Rmc.MagneticVariation;
                        if( m_NMEA0183.Rmc.MagneticVariationDirection == West )
                            var = - var;
                    }
                    NMEA_out_point.Variation = m_string.Format(_T("%5.3f"), var);

                    wxString time = m_NMEA0183.Rmc.UTCTime;
                    double value;

                    if(time.ToDouble(&value))
                    {
                        long hr = long( value / 10000);
                        long min = long(value / 100) - (hr * 100);
                        long sec = long(value) - (hr * 10000) - (min * 100);

                        UTS_time = wxString::Format(wxT("%02d:%02d:%02d NF"),hr,min,sec);
                    }

                    wxString date = m_NMEA0183.Rmc.Date;
                    if(date.ToDouble(&value))
                    {
                        long day = long( value / 10000);
                        long month = long(value / 100) - (day * 100);
                        long yr = long(value) - (day * 10000) - (month * 100);

                        UTS_date = wxString::Format(wxT("20%02d%02d%02d "),yr,month,day);
                    }
                    NMEA_out_point.UTS = UTS_date + UTS_time ;

                }
                string_parsed = true;
        }
    }
/*
    else{
                    wxLogError(_T("NMEA Sentence not being parsed \r\n %s"), sentence);
        }
*/
    return string_parsed;
}

void STE_pi::clear_variables()
{
	sum_rws = 0.0;
    count_r = 0.0;
    Wind.RWS = 0.0;

	sum_rwd = 0.0;
    Wind.RWA = -1;

	sum_tws = 0.0;
    count_t = 0.0;
    Wind.TWS = 0.0;

	sum_twd = 0.0;
    Wind.TWA = -1;
 
	Boat.SOG = 0.0;
    Boat.COG = -1;
    Boat.HDGM = -1;
	Boat.STW = 0.0;
	Boat.HDG = -1;               // don't clear var

    UTS_date.Clear();
    UTS_time.Clear();
}

wxString STE_pi::build_trt_string(void)
{
    wxString m_trt_output = 
        NMEA_out_point.UTS + _T(",") +
        NMEA_out_point.Lat + _T(",") +
        NMEA_out_point.Lon + _T(",") +
        NMEA_out_point.SOG + _T(",") +
        NMEA_out_point.COGT + _T(",") +
        NMEA_out_point.COGM + _T(",") +
        NMEA_out_point.Variation + _T(",") +
        NMEA_out_point.Depth + _T(",") +
        NMEA_out_point.CrseWind + _T(",") +
        NMEA_out_point.RelWind + _T(",") +
        NMEA_out_point.TrueWind + _T(",") +
        NMEA_out_point.TWSpd + _T(",") +
        NMEA_out_point.RWSpd + _T(",") +
        NMEA_out_point.SpdParWind + _T(",") +
        NMEA_out_point.BtHDG + _T(",") +
        NMEA_out_point.BtWatSpd + _T(",") +
        NMEA_out_point.WPLat + _T(",") +
        NMEA_out_point.WPLon + _T(",") +
        NMEA_out_point.WPRteCrse + _T(",") +
        NMEA_out_point.XTE + _T(",") +
        NMEA_out_point.BrngWP + _T(",") +
        NMEA_out_point.DistWP + _T(",") +
        NMEA_out_point.VMG + _T(",") +
        NMEA_out_point.Waypoint  + _T("\r\n");

    return m_trt_output;
}

//***************************************************************************
//******************** Graphic Display *************************************

void STE_pi::SetStart( int position )
{
    wxString m_Startstr;
    long record_count = m_istream.GetLineCount();
    
    if (end_record == 1)
        end_record = record_count;
    if (position == 0)
       start_record = 1;

    else {
        start_record = (position * record_count) / 100;
    }

    if (start_record < end_record)
    {
        m_Startstr = m_istream.GetLine(start_record);

        STE_Point Start_point;
        if (Get_Record_Data(&m_Startstr, &Start_point))
        {
            m_pSTE_Control->m_tFrom->SetValue(Start_point.UTS);
        }
    }
}

void STE_pi::SetEnd( int position )
{
    wxString m_Endstr;
    long record_count = m_istream.GetLineCount();

    end_record = (position * record_count / 100) - 1;

    if (end_record > start_record)
    {
        m_Endstr = m_istream.GetLine(end_record);

        STE_Point End_point;
        if (Get_Record_Data(&m_Endstr, &End_point))
        {
            m_pSTE_Control->m_tTo->SetValue(End_point.UTS);
        }
    } else
        end_record = start_record + 1 ;
}

STE_Point* STE_pi::Get_Record_Data(wxString* m_instr, STE_Point* m_Point)
{

    const wxChar separator = wxT(',');
    const size_t textbox_Count = 24;

    if ( m_instr->empty() )
        return false;

    wxStringTokenizer tokenizer(*m_instr, separator);
    
 //   if ( tokenizer.CountTokens() != textbox_Count )
 //       return false;

    if( tokenizer.GetPosition() == 0 )
    {
        m_Point->UTS = tokenizer.GetNextToken();
        m_Point->Lat = tokenizer.GetNextToken();
        m_Point->Lon = tokenizer.GetNextToken();
        m_Point->SOG = tokenizer.GetNextToken();
        m_Point->COGT = tokenizer.GetNextToken();
        m_Point->COGM = tokenizer.GetNextToken();
        m_Point->Variation = tokenizer.GetNextToken();
        m_Point->Depth = tokenizer.GetNextToken();
        m_Point->CrseWind = tokenizer.GetNextToken();
        m_Point->RelWind = tokenizer.GetNextToken();
        m_Point->TrueWind = tokenizer.GetNextToken();
        m_Point->TWSpd = tokenizer.GetNextToken();
        m_Point->RWSpd = tokenizer.GetNextToken();
        m_Point->SpdParWind = tokenizer.GetNextToken();
        m_Point->BtHDG = tokenizer.GetNextToken();
        m_Point->BtWatSpd = tokenizer.GetNextToken();
        m_Point->WPLat = tokenizer.GetNextToken();
        m_Point->WPLon = tokenizer.GetNextToken();
        m_Point->WPRteCrse = tokenizer.GetNextToken();
        m_Point->XTE = tokenizer.GetNextToken();
        m_Point->BrngWP = tokenizer.GetNextToken();
        m_Point->DistWP = tokenizer.GetNextToken();
        m_Point->VMG = tokenizer.GetNextToken();
        m_Point->Waypoint = tokenizer.GetNextToken();

        return m_Point;
        
    }
    return false;
}

void STE_pi::Load_track()
{
    if(!m_pTrack->m_GUID.IsEmpty()) {
        m_pTrack->pWaypointList->Clear();
        DeletePlugInTrack(m_pTrack->m_GUID);
    }

    wxString m_STE_str;
    STE_Point *m_current_STE_Point, *m_start_STE_Point;
    double m_leg_dist, prev_lat = 0, prev_lon = 0;
    
    // Track point for Track Name
    m_start_STE_Point = new STE_Point;
    
    // Set up initial display position
    m_STE_str = m_istream.GetLine(start_record);
    Get_Record_Data(&m_STE_str, m_start_STE_Point); // Set GUID data for Track

    if(m_start_STE_Point->UTS.Contains(_T("NF"))) // Note this is new form
                newform = true;
            else
                newform = false;

    prev_lat = m_start_STE_Point->GetLat();
    prev_lon = m_start_STE_Point->GetLon();

    if (!m_pSTE_PointList->empty())
    {
        m_pSTE_PointList->DeleteContents(true);
        m_pSTE_PointList->Clear();
    }

    for (long STE_record = start_record; STE_record < end_record; ++STE_record)
    {
        m_STE_str = m_istream.GetLine(STE_record);
        m_current_STE_Point = new STE_Point;
        Get_Record_Data(&m_STE_str, m_current_STE_Point);
        double m_lat = m_current_STE_Point->GetLat();
        double m_lon = m_current_STE_Point->GetLon();

        m_leg_dist = local_distance( prev_lat, prev_lon, m_lat, m_lon);
        if (m_leg_dist > track_interval)
        {            
            prev_lat = m_lat;
            prev_lon = m_lon;

            m_pSTE_PointList->Append(m_current_STE_Point);

            m_pTrack->pWaypointList->Append(STE_to_PI_Waypoint( m_current_STE_Point));         
        }
    }
// Create Route from file-track       
    m_pTrack->m_NameString = _T("STE_Track " + m_start_STE_Point->UTS);
    m_pTrack->m_GUID = _T("STE_Track "+ m_start_STE_Point->UTS);
    AddPlugInTrack( m_pTrack, false );

	start_end_change = false;
}

PlugIn_Waypoint* STE_pi::STE_to_PI_Waypoint( STE_Point *m_inpoint)
{
    wxStringTokenizer UTS_time(m_inpoint->UTS);
    wxString RecordDate = UTS_time.GetNextToken();
    wxString RecordTime = UTS_time.GetNextToken();
    wxString NameString = _("STE_" + RecordTime);
    wxString SymString = _("empty");
    wxString GuidString = m_inpoint->UTS;

    PlugIn_Waypoint *pWP = new PlugIn_Waypoint( m_inpoint->GetLat(), m_inpoint->GetLon(), SymString, NameString, GuidString );
    wxDateTime RecordDateTime = wxDateTime::Now();
    RecordDateTime.ParseTime(RecordTime);
    pWP->m_CreateTime = RecordDateTime;
    return pWP;
}


//************************************************************************
// Graphic operations


bool STE_pi::RenderOverlay(wxDC &dc, PlugIn_ViewPort *vp)
{
    if(0 == shown_dc_message) {
        shown_dc_message = 1;
        wxString message(_("The Sea Track Edit PlugIn requires OpenGL mode to be activated in Toolbox->Settings"));
	    wxMessageDialog dlg(GetOCPNCanvasWindow(),  message, _T("STE message"), wxOK);
	    dlg.ShowModal();
    }
    return false;
}

bool STE_pi::RenderGLOverlay(wxGLContext *pcontext, PlugIn_ViewPort *vp)
{
   shown_dc_message = 0;
   double wind_point_lat = 0, wind_point_lon = 0;

   if(m_pSTE_PointList)
      {
       if(!m_pSTE_PointList->IsEmpty())
          {
              if (start_end_change == true){
                  start_end_change = false;
                  Load_track();
              }

              glPushAttrib(GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_HINT_BIT);      //Save state
              glEnable(GL_BLEND);
              glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
              glPushMatrix();
              glColor4ub(0, 0, 0, 255);	// red, green, blue,  alpha (byte values)
              glLineWidth(1.0);

                STE_PointList::iterator iter;
                wxPoint pp;
                for (iter = m_pSTE_PointList->begin(); iter != m_pSTE_PointList->end(); ++iter)
                    {
                        STE_Point *wind_point = *iter;
                       
                        wind_point->TrueWind.ToDouble(&Wind.TWA);
                        if (Wind.TWA >= 0)
                        {
                            wind_point->COGT.ToDouble(&Boat.COG);
                            Wind.TWD = Wind.TWA + Boat.COG;                          
                            if(!newform) Wind.TWD = rad2deg(Wind.TWD);                      // old version had Radians

                            wind_point->TWSpd.ToDouble(&Wind.TWS);
                            if (Wind.TWS > 2)
                            {
                                GetCanvasPixLL(vp, &pp, wind_point->GetLat(), wind_point->GetLon());
                                Draw_Wind_Barb(pp, Wind.TWD, Wind.TWS);     
                            }
                        }
                    }
              glPopMatrix();
              glPopAttrib();
              RequestRefresh(m_parent_window);
              return true;
          }
          else  return false;
    }
    else  return false;
}

void STE_pi::Draw_Wind_Barb(wxPoint pp, double TWD, double speed)
{
    double rad_angle;
    double shaft_x, shaft_y;
    double barb_0_x, barb_0_y, barb_1_x, barb_1_y;
    double barb_2_x, barb_2_y ;
    double barb_legnth_0_x, barb_legnth_0_y, barb_legnth_1_x, barb_legnth_1_y;
    double barb_legnth_2_x, barb_legnth_2_y;
    double barb_legnth[18]= {
        0,0,5,                  // 5 knots
        0,0,10,
        0,5,10,
        0,10,10,                // 20 knots
        5,10,10,
        10,10,10                // 30 knots
    };

    int p = int(speed / 5);
    if (p > 5) p = 5;
    p = 3*p;

    rad_angle = deg2rad(TWD);
    
    shaft_x = cos(rad_angle) * 20;
	shaft_y = -sin(rad_angle) * 20;

    barb_0_x = pp.x + .6 * shaft_x;
    barb_0_y = (pp.y + .6 * shaft_y);
    barb_1_x = pp.x + .8 * shaft_x;
    barb_1_y = (pp.y + .8 * shaft_y);
    barb_2_x = pp.x + shaft_x;
    barb_2_y = (pp.y + shaft_y);

    
    barb_legnth_0_x = cos(rad_angle + PI/4) * barb_legnth[p];
    barb_legnth_0_y = -sin(rad_angle + PI/4) * barb_legnth[p];
    barb_legnth_1_x = cos(rad_angle + PI/4) * barb_legnth[p+1];
    barb_legnth_1_y = -sin(rad_angle + PI/4) * barb_legnth[p+1];
    barb_legnth_2_x = cos(rad_angle + PI/4) * barb_legnth[p+2];
    barb_legnth_2_y = -sin(rad_angle + PI/4) * barb_legnth[p+2];


      glBegin(GL_LINES);
        glVertex2d(pp.x, pp.y);
        glVertex2d(pp.x + shaft_x, pp.y + shaft_y);
        glVertex2d(barb_0_x, barb_0_y);
        glVertex2d(barb_0_x + barb_legnth_0_x, barb_0_y + barb_legnth_0_y);
        glVertex2d(barb_1_x, barb_1_y);
        glVertex2d(barb_1_x + barb_legnth_1_x, barb_1_y + barb_legnth_1_y);
        glVertex2d(barb_2_x, barb_2_y);
        glVertex2d(barb_2_x + barb_legnth_2_x, barb_2_y + barb_legnth_2_y);
      glEnd();
}

//----------------------------------------------------------------
//
//    STE Control Implementation
//
//----------------------------------------------------------------

STE_Control::STE_Control( wxWindow *pparent, wxWindowID id, STE_pi *STE, int start, int end, int range , wxString file_name )
      :wxWindow( pparent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE, _T("STE") ), m_pSTE(STE)
{
      wxColour cl;
      GetGlobalColor(_T("DILG1"), &cl);
      SetBackgroundColour(cl);

      wxBoxSizer *topsizer = new wxBoxSizer( wxVERTICAL );

          wxStaticText *Start_text = new wxStaticText( this, wxID_ANY, _("Start:") );
          topsizer->Add( Start_text, 0, wxEXPAND|wxALL, 2 );

          m_start_slider = new wxSlider( this, wxEVT_START_SLIDER_UPDATED, start, 1, 100, wxDefaultPosition, wxSize( 400, 20) );
          topsizer->Add( m_start_slider, 0, wxALL|wxEXPAND, 2 );
          m_start_slider->Connect( wxEVT_SCROLL_THUMBRELEASE,
              wxCommandEventHandler(STE_Control::OnStartSliderUpdated), NULL, this);

          wxStaticText *End_text = new wxStaticText( this, wxID_ANY, _("End:") );
          topsizer->Add( End_text, 0, wxEXPAND|wxALL, 2 );

          m_end_slider = new wxSlider( this, wxEVT_END_SLIDER_UPDATED, end, 1, 100, wxDefaultPosition, wxSize( 400, 20) );
          topsizer->Add( m_end_slider, 0, wxALL|wxEXPAND, 2 );
          m_end_slider->Connect( wxEVT_SCROLL_THUMBRELEASE,
              wxCommandEventHandler(STE_Control::OnEndSliderUpdated), NULL, this);
//******************************************************************************
      wxBoxSizer* bSizerName = new wxBoxSizer( wxVERTICAL );

        m_stName = new wxStaticText( this, wxID_ANY, _("Name"), wxDefaultPosition, wxDefaultSize, 0 );
        m_stName->Wrap( -1 );
        bSizerName->Add( m_stName, 0, wxALL, 2 );

        wxString record_count = wxString::Format(wxT("  %i records"),(range));
        file_name = file_name + record_count;

        m_tName = new wxTextCtrl( this, wxID_ANY, file_name, wxDefaultPosition, wxSize( 300, 8) , 0 );
        bSizerName->Add( m_tName, 1, 0, 2 );

      wxBoxSizer *bSizerFromTo = new wxBoxSizer( wxHORIZONTAL );

        m_stFrom = new wxStaticText( this, wxID_ANY, _("From"), wxDefaultPosition, wxDefaultSize, 0 );
        m_stFrom->Wrap( -1 );
        bSizerFromTo->Add( m_stFrom, 0, wxALL, 2 );
    
        m_tFrom = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
        bSizerFromTo->Add( m_tFrom, 1, 0, 2 );
    
        m_stTo = new wxStaticText( this, wxID_ANY, _("To"), wxDefaultPosition, wxDefaultSize, 0 );
        m_stTo->Wrap( -1 );
        bSizerFromTo->Add( m_stTo, 0, wxALL, 2 );
    
        m_tTo = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
        bSizerFromTo->Add( m_tTo, 1, 0, 5 );

        bSizerName->Add( bSizerFromTo, 1, wxALL|wxEXPAND, 2 );
        topsizer->Add( bSizerName, 1, wxALL|wxEXPAND, 2 );
//********************************************************************************

        sbSizerPoints = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Recorded points") ), wxVERTICAL );
/*
        m_lcPoints = new wxListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_HRULES | wxLC_VRULES | wxLC_EDIT_LABELS | wxLC_VIRTUAL );
        m_lcPoints->InsertColumn( 0, _("UTS"), wxLIST_FORMAT_CENTER, 70 );
        m_lcPoints->InsertColumn( 1, _("SOG"), wxLIST_FORMAT_CENTER, 60 );
        m_lcPoints->InsertColumn( 2, _("COGT"), wxLIST_FORMAT_CENTER, 60 );
        m_lcPoints->InsertColumn( 3, _("TWA"), wxLIST_FORMAT_CENTER, 60 );
        m_lcPoints->InsertColumn( 4, _("TWSpd"), wxLIST_FORMAT_CENTER, 60 );

        sbSizerPoints->Add( m_lcPoints, 0, wxALL|wxEXPAND, 2 );
*/        topsizer->Add( sbSizerPoints, 0, wxALL|wxEXPAND, 2 );

//*******************************************************************************
        bMakeFile = new wxButton(this, ID_MAKE_FILE, _T("Make File of Displayed Section"), wxDefaultPosition, wxDefaultSize, 0);
        topsizer->Add(bMakeFile, 0, wxALIGN_CENTER_VERTICAL | wxALL | wxEXPAND, 2);
        bMakeFile->Connect(wxEVT_COMMAND_BUTTON_CLICKED,
              wxCommandEventHandler(STE_Control::OnMakeFileButtonPush), NULL, this);
  
        SetSizer( topsizer );
        topsizer->Fit( this );
        Layout();
}

void STE_Control::OnStartSliderUpdated( wxCommandEvent& event )
{
    m_pSTE->SetStart( m_start_slider->GetValue());
    start_end_change = true;
}

void STE_Control::OnEndSliderUpdated( wxCommandEvent& event )
{
    m_pSTE->SetEnd( m_end_slider->GetValue());
    start_end_change = true;
}

void STE_Control::OnMakeFileButtonPush( wxCommandEvent& event )
{
    wxString outfilename;
    outfilename = m_ifilename;
    outfilename.RemoveLast(4);
    outfilename = outfilename + _T("ext.trt");

    wxFile outstream;
    outstream.Open( outfilename, wxFile::write );

    if (end_record == 1)
        end_record = m_istream.GetLineCount();

    if (outstream.IsOpened()) {
        for (long record = start_record; record < end_record; ++record) {
            wxString str = m_istream.GetLine(record) + _T("\r\n");
            outstream.Write(str);
        }
        outstream.Close();
    }
}

//****************************************************************************************************************************************
// Point definition to fit into track points

STE_Point::STE_Point()
{
}

STE_Point::~STE_Point(void)
{
}

double STE_Point::GetLat()
{
    double m_Lat = 0;
    if (Lat.ToDouble(&m_Lat)) {};
    return m_Lat;
}

double STE_Point::GetLon()
{
    double m_Lon = 0;
    if(Lon.ToDouble(&m_Lon)){};
    return m_Lon;
}

wxDateTime STE_Point::GetUTS()
{
    return 1.0;
}

void STE_Point::Clear(void)
{
    UTS.Clear();
    Lat.Clear();
    Lon.Clear();
    SOG = _T("0.0");
    COGT = _T("0.0");
    COGM = _T("0.0");
    Variation = _T("0.0");
    Depth = _T("0.0");
    CrseWind = _T("0.0");
    RelWind = _T("0.0");
    TrueWind = _T("0.0");
    TWSpd = _T("0.0");
    RWSpd = _T("0.0");
    SpdParWind = _T("0.0");
    BtHDG = _T("0.0");
    BtWatSpd = _T("0.0");
    WPLat = _T("0.0");
    WPLon = _T("0.0");
    WPRteCrse = _T("0.0");
    XTE = _T("0.0");
    BrngWP = _T("0.0");
    DistWP = _T("0.0");
    VMG = _T("0.0");
    Waypoint.Clear();
}
/*
//***********************************************************
// Analysis class
STE_Analysis::STE_Analysis(void)
{
}

STE_Analysis::~STE_Analysis(void)
{
}
//************************************************************
// Math equations
*/
static double deg2rad(double deg)
{
    return ((90 - deg) * PI / 180.0);
}

static double rad2deg(double rad)
{
    return (int(90 - (rad * 180.0 / PI) + 360) % 360);
}


double local_distance (double lat1, double lon1, double lat2, double lon2) {
	// Spherical Law of Cosines
	double theta, dist; 

	theta = lon2 - lon1; 
	dist = sin(lat1 * PI/180) * sin(lat2 * PI/180) + cos(lat1 * PI/180) * cos(lat2* PI/180) * cos(theta* PI/180); 
	dist = acos(dist);		// radians
	dist = dist * 180 /PI; 
    dist = fabs(dist) * 60    ;    // nautical miles/degree
	return (dist); 
}

double local_bearing (double lat1, double lon1, double lat2, double lon2) //FES
{
double angle = atan2( (lat2-lat1)*PI/180, ((lon2-lon1)* PI/180 * cos(lat1 *PI/180)));

    angle = angle * 180/PI ;
    angle = 90.0 - angle;
    if (angle < 0) angle = 360 + angle;
	return (angle);
}

double TWS(double RWS, double RWA, double SOG)
{
    RWA = deg2rad(RWA);
    double TWS_value = sqrt(pow((RWS * sin(RWA)),2) + pow((RWS * cos(RWA)- SOG),2));
    return TWS_value;
}

double TWA(double RWS, double RWA, double SOG)
{
    double RWA_R = deg2rad(RWA);
    double TWA_value = atan((RWS * sin(RWA_R))/(RWS * cos(RWA_R)-SOG));
    TWA_value = rad2deg(TWA_value);
    if(RWA > 180.0 && TWA_value < 180)
        TWA_value = TWA_value + 180;;
    if (TWA_value < 0)
        TWA_value = TWA_value + 180;
    return TWA_value;
}

double RWS(double TWS, double TWA, double SOG)
{
    TWA = deg2rad(TWA);
    double RWS_value = sqrt(pow((TWS * sin(TWA)),2) + pow((TWS * cos(TWA)+ SOG), 2));
    return RWS_value;
}

double RWA(double TWS, double TWA,double SOG)
{
    double TWA_R = deg2rad(TWA);
    double RWA_value = atan((TWS * sin(TWA_R))/(TWS * cos(TWA_R)+ SOG));
    RWA_value = rad2deg(RWA_value);
    if( TWA > 180 && RWA_value < 180)
        RWA_value = RWA_value + 180;
    if (RWA_value < 0)
        RWA_value = RWA_value + 180;
    return RWA_value;
}
