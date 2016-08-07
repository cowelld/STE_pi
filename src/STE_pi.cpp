/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  STE Plugin
 * Author:   Dave Cowell
 *
 ***************************************************************************
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
#include <wx/notebook.h>
#include <wx/glcanvas.h>
//#include <wx/listctrl.h>
#include <list>
#include <wx/tokenzr.h>
#include "STE_pi.h"
#include "icons.h"
#include "ocpn_plugin.h"

using namespace std;

#ifdef __WXMSW__
#include "GL/glu.h"
#endif

bool  shown_dc_message;
bool initialized = false;
double pos_lat, pos_lon;
double cur_lat, cur_lon;
double track_lat, track_lon;
double uvp_scale;
bool bSTE_point_selected = false;

long start_record = 1, end_record = 1, record_count;
bool start_end_change = true;
bool data_edited = false;bool need_new_rendering = true;

int FileType = 0; // trt file
double track_interval;
int track_int_sel;
bool newform;
wxTextFile   m_istream;
STE_Point    NMEA_out_point;

#include <wx/listimpl.cpp>
WX_DEFINE_LIST(STE_PointList);

long last_item;

STE_PointList *m_pSTE_PointList = NULL;
PlugIn_Track *m_pTrack = NULL;

double sum_rws, count_r, sum_rwa;
double sum_tws, count_t, sum_twa;
double var;
wxString UTS_time, UTS_date, record_date;


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
      m_pGraph = NULL;
      m_PlugIn_STE = InsertPlugInTool(_T(""), _img_STE_play, _img_STE_play, wxITEM_CHECK,
            _("STE"), _T(""), NULL, STE_TOOL_POSITION, 0, this);

      return (
           WANTS_TOOLBAR_CALLBACK    |
           INSTALLS_TOOLBAR_TOOL     |
           WANTS_CONFIG              |
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
         if (!m_pSTE_PointList->IsEmpty())
        {
            m_pSTE_PointList->DeleteContents(true);
            m_pSTE_PointList->Clear();
            delete m_pSTE_PointList;
        }

     if (m_pTrack){
         if (!m_pTrack->pWaypointList->IsEmpty())
        {
            m_pTrack->pWaypointList->DeleteContents(true);
            DeletePlugInTrack(m_pTrack->m_GUID);
        }
     }

     if (m_pGraph)
     {
         m_pauimgr->DetachPane(m_pGraph);
         m_pGraph->Close();
         m_pGraph->Destroy();
         m_pGraph = NULL;
     }

     if (m_istream.IsOpened())
    {
        m_istream.Close();
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
          if (!initialized && !m_istream.IsOpened() )
          {
               SetToolbarItemState( m_PlugIn_STE, true );

        //************ Open file for data *******************************
                wxString message = _("Select track (trt) or log (txt) file");
                wxString filetypext = _("*.t*");
                wxFileDialog fdlg( m_parent_window, message , _T(""), m_ifilename, filetypext, wxFD_OPEN|wxFD_FILE_MUST_EXIST );
                if ( fdlg.ShowModal() != wxID_OK)
                    {
                        SetToolbarItemState( m_PlugIn_STE, false );
                        initialized = false;
                        return;
                    }

                m_ifilename.Clear();
                m_ifilename = fdlg.GetPath();

        //********************** Create trt file from NMEA message file (txt) ************************

                if (m_ifilename.AfterLast('.') == _("txt"))
                {
                    if(!Loadtxt_trt())                      // convert file and rename m_ifilename
                    {
                        SetToolbarItemState( m_PlugIn_STE, false );
                        initialized = false;
                    }
                }

        //******************** Open trt file and load data ****************
                else
                {
                    m_istream.Open( m_ifilename );
                    record_count = m_istream.GetLineCount();

                    if (!m_pSTE_Control )
                    {

                        m_pSTE_Control = new STE_Control( m_parent_window, wxID_ANY, this, 1, 100, m_istream.GetLineCount(), m_ifilename);
                        wxAuiPaneInfo pane1 = wxAuiPaneInfo().Name(_T("STE")).Caption(_("STE Control")).CaptionVisible(true).Float().FloatingPosition(50,100).Dockable(false).Fixed().CloseButton(false).Show(true);
                        m_pauimgr->AddPane( m_pSTE_Control, pane1 );
                        m_pauimgr->Update();

                    }

                    m_pTrack = new PlugIn_Track;
                    m_pSTE_PointList = new STE_PointList;

                    SetStart(0);
                    SetEnd(100);
                    Load_track();

                    long point_count = m_pSTE_PointList->GetCount();
                    m_pSTE_Control->m_lcPoints->m_pSTE_PointList = m_pSTE_PointList;
                    m_pSTE_Control->m_lcPoints->SetItemCount(point_count);
                    need_new_rendering = true;
//************data analysis ***********************************************
                    if(!m_pAnalysis)
                    {
                        m_pAnalysis = new STE_Analysis();
                    }

//************ Data graphical display *************************************
/*
                    if (!m_pGraph )
                    {
                        m_pGraph = new STE_Graph( m_parent_window, wxID_ANY, this, wxDefaultPosition, wxSize(300,200));
                        wxAuiPaneInfo pane2 = wxAuiPaneInfo().Name(_T("STE_Graph")).Caption(_("STE Graph")).CaptionVisible(true).Float().FloatingPosition(100,500).Dockable(false).Fixed().CloseButton(false).Show(true);
                        m_pauimgr->AddPane( m_pGraph, pane2 );
                        m_pauimgr->Update();
                    }
 */               initialized = true;
                }
            }
//*********** De-select Plug-in ******************************
            else
            {
               SetToolbarItemState( m_PlugIn_STE, false );

               if ( m_pSTE_Control )
                      {
                        m_pauimgr->DetachPane( m_pSTE_Control );
                        m_pSTE_Control->Close();
                        m_pSTE_Control->Destroy();
                        m_pSTE_Control = NULL;
                      }
               if (m_pSTE_PointList)
                   if (!m_pSTE_PointList->IsEmpty())
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

                if(m_pAnalysis)
                     {
                         m_pAnalysis->~STE_Analysis();
                     }

                if (m_pGraph)
                     {
                         m_pauimgr->DetachPane(m_pGraph);
                         m_pGraph->Close();
                         m_pGraph->Destroy();
                         m_pGraph = NULL;
                //         m_graph->Disconnect(wxEVT_PAINT, wxPaintEventHandler( Graph::OnPaint ) );
                //        delete m_graph;
                     }

                 if (m_istream.IsOpened())
                    {
                        m_istream.Close();
                    }
                initialized = false;                need_new_rendering = false;
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
    pStatic_var = new wxStaticText(dialog, wxID_ANY, _("Magnetic Variation"));
    BoxSizerSTE->Add(pStatic_var, 1, wxALIGN_LEFT | wxALL, 2);

    wxString t_var = wxString::Format(_("%3.2f"), var);    m_pvar = new wxTextCtrl( dialog, wxID_ANY);
	BoxSizerSTE->Add( m_pvar, 0, wxALL|wxALIGN_LEFT, 5 );
    m_pvar->SetValue(t_var);
    wxStdDialogButtonSizer* DialogButtonSizer = dialog->CreateStdDialogButtonSizer(wxOK|wxCANCEL);
    Display_Preferencs_panel->Add(DialogButtonSizer, 0, wxALIGN_RIGHT|wxALL, 5);

	dialog->Fit();

//********************** Input Data selections by modal ********************
    if(dialog->ShowModal() == wxID_OK)  // Use this instead of Event driven routines.
      {
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
	    }	    if(t_var != m_pvar->GetValue()){
                t_var = m_pvar->GetValue();
                t_var.ToDouble(&var);
	    }
        SaveConfig();
	}
}

bool STE_pi::LoadConfig(void)
{
      wxFileConfig *pConf = (wxFileConfig *)m_pconfig;

      if(pConf)
      {
            pConf->SetPath( _T("/PlugIns/STE") );
            pConf->Read( _T("TrackInterval"), &track_interval, .1 );            pConf->Read( _T("Variation"), &var, -14.0);
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
            pConf->Write( _T("Variation"), var);
            return true;
      }
      else
            return false;
}

void STE_pi::SetCursorLatLon(double lat, double lon)
{
cur_lat = lat;
cur_lon = lon;
}


//***************************************************************
//      Local functions
//***************************************************************
bool STE_pi::Loadtxt_trt( void )
{
    wxTextFile instream;
    wxFile outstream;
    wxString outfilename, outfiledir, daily_filename, sentence, log_date = _T("");

    outfilename.Clear();
    outfilename = m_ifilename;
    outfiledir = outfilename.BeforeLast('/') + _T('/');

    instream.Open( m_ifilename );
    double file_size = (double)instream.GetLineCount();
    if (file_size > 10000)
    {
        wxString m_string = m_string.Format(_(" This file has %5.0f records. Continue?"), file_size); // instream.GetLineCount()));
        int do_file = wxMessageBox(m_string, _(""), wxYES_NO);
        if(do_file == wxNO) return false;
    }

    wxString n_string = n_string.Format(_(" Make into Daily logs?"));
    int do_dailylog = wxMessageBox(n_string, _(""), wxYES_NO);

    if(do_dailylog == wxNO){
    	outfilename.RemoveLast(2);
    	outfilename = outfilename + _T("rt");
    	outstream.Open( outfilename, wxFile::write );

    	while (!instream.Eof())
    	{
    	    sentence = instream.GetNextLine();

    	    if (outstream.IsOpened()){
    	         if( make_trt_line (sentence)){
    	                wxString trt_line = build_trt_string(&NMEA_out_point);
    	                outstream.Write(trt_line);
    	                NMEA_out_point.Clear();             // Clear Output string template
    	                clear_variables();
    	          }
    	    }
    	}
    }
    else {
    	while (!instream.Eof())
    	{
            sentence = instream.GetNextLine();

            if (record_date != log_date){
            	log_date = record_date;
            	outstream.Close();
            	daily_filename = outfiledir + log_date + _T(".trt");
            	outstream.Open( daily_filename, wxFile::write );
            }

            if( make_trt_line (sentence)){
                wxString trt_line = build_trt_string(&NMEA_out_point);
                outstream.Write(trt_line);
                NMEA_out_point.Clear();             // Clear Output string template
                clear_variables();
            }
    	}
    }
    outstream.Close();
    instream.Close();
    return true;
}

bool STE_pi::make_trt_line(wxString sentence)   // test and fill in true wind data
{
    bool make_OK = false;
    wxString m_string;
    if(NMEA_parse(sentence))
    {

        if (Boat.SOG > 1.0 && Boat.COG >= 0.0 ){

            if (Wind.RWA >= 0.0 && Wind.RWS > 2.0 && Wind.TWS <= 0.0)
            {
                Wind.TWS = TWS(Wind.RWS,Wind.RWA,Boat.SOG);
                Wind.TWA = TWA(Wind.RWS,Wind.RWA,Boat.SOG);

                NMEA_out_point.TWA = m_string.Format(_T("%3.1f"), Wind.TWA);
                NMEA_out_point.TWS = m_string.Format(_T("%3.1f"), Wind.TWS);
            }

            if (Wind.TWA >= 0.0 && Wind.TWS > 2.0 && Wind.RWS <= 0.0)
            {
                Wind.RWS = RWS(Wind.TWS,Wind.TWA,Boat.SOG);
                Wind.RWA = RWA(Wind.TWS,Wind.TWA,Boat.SOG);

                NMEA_out_point.RWA = m_string.Format(_T("%3.1f"), Wind.RWA);
                NMEA_out_point.RWS = m_string.Format(_T("%3.1f"), Wind.RWS);
            }

            Wind.TWD = Wind.TWA + Boat.COG;
            if (Wind.TWD > 360) Wind.TWD = Wind.TWD - 360;
            NMEA_out_point.TWD =  m_string.Format(_T("%3.1f"), Wind.TWD);
        }

        if(!NMEA_out_point.UTS.IsEmpty()){
           if(!NMEA_out_point.Lat.IsEmpty() && !NMEA_out_point.Lon.IsEmpty()){
                if(!NMEA_out_point.SOG.IsEmpty() && !NMEA_out_point.COGT.IsEmpty()){
                    if(!NMEA_out_point.RWA.IsEmpty() && !NMEA_out_point.RWS.IsEmpty())
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
            NMEA_out_point.Variation = m_string.Format(_T("%3.1f"), var);

            if( !wxIsNaN( m_NMEA0183.Hdg.MagneticSensorHeadingDegrees ) )
            {
                Boat.HDG = m_NMEA0183.Hdg.MagneticSensorHeadingDegrees + var;
                if (Boat.HDG > 360) Boat.HDG = Boat.HDG - 360;
                NMEA_out_point.BtHDG = m_string.Format(_T("%3.1f"), Boat.HDG);
            }
            string_parsed = true;
        }

        else if( m_NMEA0183.LastSentenceIDParsed == _T("HDM") )
        {
            if( !wxIsNaN( m_NMEA0183.Hdm.DegreesMagnetic) )
            {
                Boat.HDG = m_NMEA0183.Hdm.DegreesMagnetic + var;
                NMEA_out_point.BtHDG = m_string.Format(_T("%3.1f"), Boat.HDG);
            }
            string_parsed = true;
        }

        else if( m_NMEA0183.LastSentenceIDParsed == _T("HDT") )
        {
            if( !wxIsNaN( m_NMEA0183.Hdt.DegreesTrue) )
            {
                Boat.HDG = m_NMEA0183.Hdt.DegreesTrue;
                NMEA_out_point.BtHDG = m_string.Format(_T("%3.1f"), Boat.HDG);
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
                    sum_rwa = sum_rwa + m_NMEA0183.Mwv.WindAngle;
                    count_r = count_r + 1;
                    sum_rws =  sum_rws + wind_speed;

                    Wind.RWA = sum_rwa/count_r;
                    Wind.RWS = sum_rws/count_r;
                    NMEA_out_point.RWA = m_string.Format(_T("%3.1f"), Wind.RWA);
                    NMEA_out_point.RWS = m_string.Format(_T("%3.1f"), Wind.RWS);
                }
                else
                {
                    sum_twa = sum_twa + m_NMEA0183.Mwv.WindAngle;
                    count_t = count_t + 1;
                    sum_tws = sum_tws + wind_speed;

                    Wind.TWA = sum_twa/count_t;
                    Wind.TWS = sum_tws/count_t;
                    NMEA_out_point.TWA = m_string.Format(_T("%3.1f"), Wind.TWA);
                    NMEA_out_point.TWS = m_string.Format(_T("%3.1f"), Wind.TWS);
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
                        NMEA_out_point.COGT = m_string.Format(_T("%3.1f"), Boat.COG);
                    }

                    if( !wxIsNaN(m_NMEA0183.Rmc.MagneticVariation) )
                    {
                        var = m_NMEA0183.Rmc.MagneticVariation;
                        if( m_NMEA0183.Rmc.MagneticVariationDirection == West )
                            var = - var;
                    }
                    NMEA_out_point.Variation = m_string.Format(_T("%3.1f"), var);

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
                        record_date = UTS_date;
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

	sum_rwa = 0.0;
    Wind.RWA = -1;

	sum_tws = 0.0;
    count_t = 0.0;
    Wind.TWS = 0.0;

	sum_twa = 0.0;
    Wind.TWA = -1;
    Wind.TWD = -1;

	Boat.SOG = 0.0;
    Boat.COG = -1;
    Boat.HDGM = -1;
	Boat.STW = 0.0;
	Boat.HDG = -1;               // don't clear var

    UTS_date.Clear();
    UTS_time.Clear();
}

wxString STE_pi::build_trt_string(STE_Point *Point)
{
    wxString output =
        Point->UTS + _T(",") +
        Point->Lat + _T(",") +
        Point->Lon + _T(",") +
        Point->SOG + _T(",") +
        Point->COGT + _T(",") +
        Point->COGM + _T(",") +
        Point->Variation + _T(",") +
        Point->Depth + _T(",") +
        Point->TWD + _T(",") +
        Point->RWA + _T(",") +
        Point->TWA + _T(",") +
        Point->TWS + _T(",") +
        Point->RWS + _T(",") +
        Point->VMG_W + _T(",") +
        Point->BtHDG + _T(",") +
        Point->BtWatSpd + _T(",") +
        Point->WPLat + _T(",") +
        Point->WPLon + _T(",") +
        Point->WPRteCrse + _T(",") +
        Point->XTE + _T(",") +
        Point->BrngWP + _T(",") +
        Point->DistWP + _T(",") +
        Point->VMG_C + _T(",") +
        Point->Waypoint  + _T("\r\n");

    return output;
}

//***************************************************************************
//******************** Track Data Operations *************************************

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

bool STE_pi::Get_Record_Data(wxString* m_instr, STE_Point* m_Point)
{

    const wxChar separator = wxT(',');
    const size_t textbox_Count = 24;

    if ( m_instr->empty() )
        return false;

    wxStringTokenizer tokenizer(*m_instr, separator);

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
        m_Point->TWD = tokenizer.GetNextToken();
        m_Point->RWA = tokenizer.GetNextToken();
        m_Point->TWA = tokenizer.GetNextToken();
        m_Point->TWS = tokenizer.GetNextToken();
        m_Point->RWS = tokenizer.GetNextToken();
        m_Point->VMG_W = tokenizer.GetNextToken();
        m_Point->BtHDG = tokenizer.GetNextToken();
        m_Point->BtWatSpd = tokenizer.GetNextToken();
        m_Point->WPLat = tokenizer.GetNextToken();
        m_Point->WPLon = tokenizer.GetNextToken();
        m_Point->WPRteCrse = tokenizer.GetNextToken();
        m_Point->XTE = tokenizer.GetNextToken();
        m_Point->BrngWP = tokenizer.GetNextToken();
        m_Point->DistWP = tokenizer.GetNextToken();
        m_Point->VMG_C = tokenizer.GetNextToken();
        m_Point->Waypoint = tokenizer.GetNextToken();

        return true;

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

    if (!m_pSTE_PointList->IsEmpty())
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
    m_pTrack->m_GUID = _T("STE_Track " + m_start_STE_Point->UTS);
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
// ************************* Chart Render Operations


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
   double wind_point_lat = 0, wind_point_lon = 0;    bool updated = false;
   uvp_scale = vp->view_scale_ppm;
   if ( need_new_rendering )   {
       if(m_pSTE_PointList)
          {
           if(!m_pSTE_PointList->IsEmpty())
              {
                  if (start_end_change == true)
                  {
                      Load_track();
                      start_end_change = false;

                      long point_count = m_pSTE_PointList->GetCount();

                      m_pSTE_Control->m_lcPoints->m_pSTE_PointList = m_pSTE_PointList;
                      m_pSTE_Control->m_lcPoints->SetItemCount(point_count);
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
                            clear_variables();
                            wind_point->TWD.ToDouble(&Wind.TWD);
                            if (Wind.TWD < 0)
                            {
                                wind_point->COGT.ToDouble(&Boat.COG);
                                wind_point->TWA.ToDouble(&Wind.TWA);
                                Wind.TWD = Wind.TWA + Boat.COG;
                            }

                            if(!newform) Wind.TWD = rad2deg(Wind.TWD);      // old version had Radians
                            if(Wind.TWD > 360) Wind.TWD = Wind.TWD - 360;

                            wind_point->TWS.ToDouble(&Wind.TWS);
                            if (Wind.TWS > 1)
                            {
                                GetCanvasPixLL(vp, &pp, wind_point->GetLat(), wind_point->GetLon());
                                Draw_Wind_Barb(pp, Wind.TWD, Wind.TWS);
                            }

                        }
                    if(bSTE_point_selected){
                        GetCanvasPixLL(vp, &pp, track_lat, track_lon);
                        DrawCircle(pp, 10, 30);
                    }

                  glPopMatrix();
                  glPopAttrib();
                  RequestRefresh(m_parent_window);
                  updated = true;
              }
        }
        else need_new_rendering = false;    }    return updated;
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

void STE_pi::DrawCircle(wxPoint pp, float r, int num_segments)
{
    glBegin(GL_LINE_LOOP);
    for(int ii = 0; ii < num_segments; ii++)
    {
        float theta = 2.0f * 3.1415926f * float(ii) / float(num_segments);//get the current angle

        float x = r * cosf(theta);//calculate the x component
        float y = r * sinf(theta);//calculate the y component

        glVertex2f(x + pp.x, y + pp.y);//output vertex

    }
    glEnd();
}
//----------------------------------------------------------------
//    STE Control Implementation
//----------------------------------------------------------------

STE_Control::STE_Control( wxWindow *pparent, wxWindowID id, STE_pi *STE, int start, int end, int range , wxString file_name )
      :wxWindow( pparent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE, _T("STE Control") ), m_pSTE(STE)
{
      wxColour cl;
      GetGlobalColor(_T("DILG1"), &cl);
      SetBackgroundColour(cl);

      topsizer = new wxBoxSizer( wxVERTICAL );

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
        m_lcPoints = new STE_PointListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_HRULES | wxLC_VRULES | wxLC_VIRTUAL );

        m_lcPoints->InsertColumn( 0, _("UTS"), wxLIST_FORMAT_CENTER, 120 );
        m_lcPoints->InsertColumn( 1, _("SOG"), wxLIST_FORMAT_CENTER, 40 );
        m_lcPoints->InsertColumn( 2, _("COG"), wxLIST_FORMAT_CENTER, 40 );
        m_lcPoints->InsertColumn( 3, _("TWA"), wxLIST_FORMAT_CENTER, 40 );
        m_lcPoints->InsertColumn( 4, _("TWS"), wxLIST_FORMAT_CENTER, 40 );
        m_lcPoints->InsertColumn( 5, _("RWA"), wxLIST_FORMAT_CENTER, 40 );

        m_lcPoints->SetMinSize(wxSize(-1, 200) );

        sbSizerPoints->Add( m_lcPoints, 0, wxALL|wxEXPAND, 2 );
        topsizer->Add( sbSizerPoints, 0, wxALL|wxEXPAND, 2 );
        m_lcPoints->Connect(wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( STE_Control::OnTrackPropListClick ), NULL, this );

//*******************************************************************************
        bValidate_wind = new wxButton(this, ID_MAKE_FILE, _T("Validate Wind Calculations"), wxDefaultPosition, wxDefaultSize, 0);
        topsizer->Add(bValidate_wind, 0, wxALIGN_CENTER_VERTICAL | wxALL | wxEXPAND, 2);
        bValidate_wind->Connect(wxEVT_COMMAND_BUTTON_CLICKED,
              wxCommandEventHandler(STE_Control::OnValidate_windButtonPush), NULL, this);

        bMakeFile = new wxButton(this, ID_MAKE_FILE, _T("Make File of Displayed Section"), wxDefaultPosition, wxDefaultSize, 0);
        topsizer->Add(bMakeFile, 0, wxALIGN_CENTER_VERTICAL | wxALL | wxEXPAND, 2);
        bMakeFile->Connect(wxEVT_COMMAND_BUTTON_CLICKED,
              wxCommandEventHandler(STE_Control::OnMakeFileButtonPush), NULL, this);

        SetSizer( topsizer );
        topsizer->Fit( this );
        Layout();
}

STE_Control::~STE_Control()
{
        m_lcPoints->Disconnect(wxEVT_COMMAND_LIST_ITEM_SELECTED,
            wxListEventHandler( STE_Control::OnTrackPropListClick ), NULL, this );

        m_start_slider->Disconnect( wxEVT_SCROLL_THUMBRELEASE,
              wxCommandEventHandler(STE_Control::OnStartSliderUpdated), NULL, this);

        m_end_slider->Disconnect( wxEVT_SCROLL_THUMBRELEASE,
              wxCommandEventHandler(STE_Control::OnEndSliderUpdated), NULL, this);

        bValidate_wind->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED,
              wxCommandEventHandler(STE_Control::OnValidate_windButtonPush), NULL, this);

        bMakeFile->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED,
              wxCommandEventHandler(STE_Control::OnMakeFileButtonPush), NULL, this);
}

void STE_Control::OnStartSliderUpdated( wxCommandEvent& event )
{
    m_pSTE->SetStart( m_start_slider->GetValue());
    start_end_change = true;    need_new_rendering = true;
}

void STE_Control::OnEndSliderUpdated( wxCommandEvent& event )
{
    m_pSTE->SetEnd( m_end_slider->GetValue());
    start_end_change = true;    need_new_rendering = true;
}

void STE_Control::OnMakeFileButtonPush( wxCommandEvent& event )
{

    wxString outfilename;
    outfilename = m_ifilename;
    outfilename.RemoveLast(4);
    outfilename = outfilename + _T("a.trt");

    wxFile outstream;
    outstream.Open( outfilename, wxFile::write );
    if(data_edited)
    {
        STE_PointList::iterator iter;
           for (iter = m_pSTE_PointList->begin(); iter != m_pSTE_PointList->end(); ++iter)
           {
               STE_Point *out_point = *iter;
                wxString str = this->m_pSTE->build_trt_string(out_point);
                outstream.Write(str);
           }
           outstream.Close();
    }
    else
    {
        if (end_record == 1)
            end_record = m_istream.GetLineCount();

        if (outstream.IsOpened()) {
            for (long record = start_record; record < end_record; record++) {
                wxString str = m_istream.GetLine(record) + _T("\r\n");
                outstream.Write(str);
            }
            outstream.Close();
    }
    }
}

void STE_Control::OnValidate_windButtonPush(wxCommandEvent& event)
{
    double temp;
    wxString temp_str;

    if(m_pSTE_PointList && !m_pSTE_PointList->IsEmpty())
       {
           data_edited = true;
           newform = true;
           STE_PointList::iterator iter;
           for (iter = m_pSTE_PointList->begin(); iter != m_pSTE_PointList->end(); ++iter)
            {
                    STE_Point *val_point = *iter;
                    if(val_point->UTS.Contains(_T("NF")))
                    {
                        val_point->COGT.ToDouble(&Boat.COG);
                        val_point->SOG.ToDouble(&Boat.SOG);
                        val_point->RWS.ToDouble(&Wind.RWS);
                        val_point->RWA.ToDouble(&Wind.RWA);

                        temp = TWA(Wind.RWS, Wind.RWA, Boat.SOG);
                        val_point->TWA = temp_str.Format(_("%3.1f"),temp);

                        temp = TWS(Wind.RWS, Wind.RWA, Boat.SOG);
                        val_point->TWS = temp_str.Format(_("%3.1f"),temp);

                        temp = abs(Boat.SOG * cos(Wind.TWA * PI/180));
                        val_point->VMG_W = temp_str.Format(_("%3.1f"),temp);
                    }
                    else
                    {
                        val_point->UTS = val_point->UTS + _("NF");

                        val_point->BtHDG.ToDouble(&Boat.HDG);
                        Boat.HDG = Boat.HDG * 180/PI;
                        val_point->BtHDG = temp_str.Format(_("%3.1f"),Boat.HDG);

                        val_point->COGM.ToDouble(&Boat.COG);
                        Boat.COG = Boat.COG * 180/PI;
                        val_point->COGM = temp_str.Format(_("%3.1f"),Boat.COG);

                        val_point->COGT.ToDouble(&Boat.COG);
                        Boat.COG = Boat.COG * 180/PI;
                        val_point->COGT = temp_str.Format(_("%3.1f"),Boat.COG);

                        val_point->Variation.ToDouble(&temp);
                        temp = temp * 180/PI;
                        val_point->Variation = temp_str.Format(_("%3.1f"),temp);

                        val_point->RWA.ToDouble(&Wind.RWA);
                        Wind.RWA = Wind.RWA * 180/PI;
                        val_point->RWA = temp_str.Format(_("%3.1f"),Wind.RWA);

                        val_point->SOG.ToDouble(&Boat.SOG);
                        val_point->RWS.ToDouble(&Wind.RWS);

                        temp = TWA(Wind.RWS, Wind.RWA, Boat.SOG);
                        val_point->TWA = temp_str.Format(_("%3.1f"),temp);

                        temp = TWS(Wind.RWS, Wind.RWA, Boat.SOG);
                        val_point->TWS = temp_str.Format(_("%3.1f"),temp);

                        temp = Wind.TWA + Boat.COG;
                        if (temp > 360) temp = temp-360;
                        val_point->TWD = temp_str.Format(_("%3.1f"),temp);

                        temp = abs( Boat.SOG * cos(Wind.TWA * PI/180));
                        val_point->VMG_W = temp_str.Format(_("%3.1f"),temp);
                    }
            }            need_new_rendering = true;
       }
}

void STE_Control::OnTrackPropListClick( wxListEvent& event )
{
    long itemno = -1;

    int selected_no;
    itemno = m_lcPoints->GetNextItem( itemno, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
    if( itemno == -1 )
        selected_no = 0;
    else
        selected_no = itemno;
   // wxPoint pp;
    wxString Lat = m_lcPoints->OnGetItemText(itemno, 6);
    wxString Lon = m_lcPoints->OnGetItemText(itemno, 7);

    track_lat = wxAtof(Lat);
    track_lon = wxAtof(Lon);
    JumpToPosition(track_lat, track_lon, uvp_scale);
    bSTE_point_selected = true;    need_new_rendering = true;
}

//*****************************************************************
//**************** Data Analysis **********************************
STE_Analysis::STE_Analysis(void)
{
    STE_PointList::iterator iter;
    max_TWS = 1;
    max_SOG = 1;

    for (iter = m_pSTE_PointList->begin(); iter != m_pSTE_PointList->end(); ++iter)
    {
        STE_Point *temp_point = *iter;

        double SOG;
        temp_point->SOG.ToDouble(&SOG);
        if ( SOG > max_SOG) max_SOG = SOG;
        double TWS;
        temp_point->TWS.ToDouble(&TWS);
        if (TWS > max_TWS)
        {
            max_TWS = TWS;
            long TWD;
            temp_point->TWD.ToLong(&TWD);
            max_TWD = TWD;
        }

    }
}
STE_Analysis::~STE_Analysis(void)
{
}


//----------------------------------------------------------------
//    STE Graph Implementation
//----------------------------------------------------------------

STE_Graph::STE_Graph( wxWindow *pparent, wxWindowID id, STE_pi *STE, wxPoint Position, wxSize Size)
      :wxWindow( pparent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE, _T("STE Graph") ), m_pSTE(STE)
{
    wxColour cl;
    GetGlobalColor(_T("DILG1"), &cl);
    SetBackgroundColour(cl);

    wxBoxSizer* bSizerNotebook;
	bSizerNotebook = new wxBoxSizer( wxVERTICAL );

    m_notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition,wxSize(1100,850), 0 );

//***************************************************************************************
//********************** Wind Page ******************************************************
        m_Plot1 = new mpWindow(m_notebook,wxID_ANY,wxDefaultPosition,wxSize(1000,800) );
        mpScaleX* x1_axis = new mpScaleX(wxT("Time"), mpALIGN_BOTTOM, true, mpX_TIME);
        mpScaleY* y1_axis = new mpScaleY(wxT("True Wind Direction"), mpALIGN_LEFT, true);


        wxFont *graphFont = wxTheFontList->FindOrCreateFont(10, wxDEFAULT, wxNORMAL, wxBOLD );
        x1_axis->SetFont(*graphFont);
        y1_axis->SetFont(*graphFont);
        x1_axis->SetDrawOutsideMargins(false);
        y1_axis->SetDrawOutsideMargins(false);
        x1_axis->SetLabelFormat(wxT("%3.0f"));
	    y1_axis->SetLabelFormat(wxT("%3.0f"));
        m_Plot1->SetMargins(10, 10, 20, 20);
        m_Plot1->AddLayer( x1_axis );
        m_Plot1->AddLayer( y1_axis );

        STE_Point *first_point = m_pSTE_PointList->GetFirst()->GetData();
        wxDateTime first_date = first_point->GetUTS();

        STE_Point *last_point = m_pSTE_PointList->GetLast()->GetData();
        wxDateTime last_date = last_point->GetUTS();
        double first_ticks = first_date.GetTicks();
        double last_ticks = last_date.GetTicks();
        double delta_ticks = last_ticks - first_ticks;

        if (delta_ticks > 0)
        {
            STE_PointList::iterator iter;
            for (iter = m_pSTE_PointList->begin(); iter != m_pSTE_PointList->end(); ++iter)
            {
                STE_Point *wind_point = *iter;
                wind_point->TWS.ToDouble(&Wind.TWS);
                wind_point->TWD.ToDouble(&Wind.TWD);
                if (Wind.TWS > 2)
                {
                    double scale_wind = Wind.TWS / 20; //STE->m_pAnalysis->max_TWS;
                    vector_y1.push_back(scale_wind);

                    double scale_direction = Wind.TWD / 360;
                    if(!newform) scale_direction = Wind.TWD/2*PI;
                    vector_y2.push_back(scale_direction);

                    wxDateTime x_date = wind_point->GetUTS();
                    double current_ticks = x_date.GetTicks();
                    vector_x1.push_back( (current_ticks - first_ticks)/ delta_ticks);
                }
            }
        }

        mpFXYVector* vectorLayer11 = new mpFXYVector(_("Wind Speed"));
        vectorLayer11->SetData(vector_x1, vector_y1);
	    vectorLayer11->SetContinuity(true);
	    wxPen vectorpen(*wxBLUE, 2, wxSOLID);
	    vectorLayer11->SetPen(vectorpen);
        m_Plot1->AddLayer(vectorLayer11, true);

        mpFXYVector* vectorLayer12 = new mpFXYVector(_("Wind Direction"));
        vectorLayer12->SetData(vector_x1, vector_y2);
	    vectorLayer12->SetContinuity(true);
	    vectorpen.SetColour(*wxRED);
	    vectorLayer12->SetPen(vectorpen);
        m_Plot1->AddLayer(vectorLayer12, true);

        m_Legend1 = new  mpInfoLegend(wxRect(200,20,100,40), wxTRANSPARENT_BRUSH);
        m_Legend1->SetName(_("Wind Direction & Speed"));
        m_Legend1->SetVisible(true);
        m_Plot1->AddLayer( m_Legend1);

    m_notebook->AddPage( m_Plot1, _("Wind"), true );

//***************************************************************************************
//********************** Wind Page ******************************************************
    m_Plot2 = new mpWindow(m_notebook,wxID_ANY,wxDefaultPosition,wxSize(1000,800) );
        mpScaleX* x2_axis = new mpScaleX(wxT("Angle"), mpALIGN_BOTTOM, true, mpX_NORMAL);
        mpScaleY* y2_axis = new mpScaleY(wxT("Boat Speed"), mpALIGN_CENTER, true);

        x2_axis->SetFont(*graphFont);
        y2_axis->SetFont(*graphFont);
        x2_axis->SetDrawOutsideMargins(false);
        y2_axis->SetDrawOutsideMargins(false);
        x2_axis->SetLabelFormat(wxT("%3.0f"));
	    y2_axis->SetLabelFormat(wxT("%3.0f"));
        m_Plot2->SetMargins(10, 10, 20, 20);
        m_Plot2->AddLayer( x2_axis );
        m_Plot2->AddLayer( y2_axis );

        mpFXYVector* vectorLayer21 = new mpFXYVector(_("Boat Speed"));
        vectorLayer21->SetData(vector_x2, vector_y3);
	    vectorLayer21->SetContinuity(true);
	    vectorpen.SetColour(*wxBLUE);
	    vectorLayer21->SetPen(vectorpen);
        m_Plot2->AddLayer(vectorLayer21, true);

        mpFXYVector* vectorLayer22 = new mpFXYVector(_("Wind Direction"));
        vectorLayer22->SetData(vector_x2, vector_y3);
	    vectorLayer22->SetContinuity(true);
	    vectorpen.SetColour(*wxRED);
	    vectorLayer22->SetPen(vectorpen);
        m_Plot2->AddLayer(vectorLayer22, true);

        m_Legend2 = new  mpInfoLegend(wxRect(200,20,100,40), wxTRANSPARENT_BRUSH);
        m_Legend2->SetName(_("Wind Direction & Speed"));
        m_Legend2->SetVisible(true);
        m_Plot2->AddLayer( m_Legend2);

      m_notebook->AddPage( m_Plot2, _("Tacks"), false );

//***************************************************************************************
//********************** Other Page ******************************************************
      m_Plot3 = new mpWindow(m_notebook,wxID_ANY,wxDefaultPosition,wxSize(1000,800) );
        mpScaleX* x3_axis = new mpScaleX(wxT("Angle"), mpALIGN_CENTER, true, mpX_NORMAL);
        mpScaleY* y3_axis = new mpScaleY(wxT("Boat Speed"), mpALIGN_LEFT, true);

        x3_axis->SetFont(*graphFont);
        y3_axis->SetFont(*graphFont);
        x3_axis->SetDrawOutsideMargins(false);
        y3_axis->SetDrawOutsideMargins(false);
        x3_axis->SetLabelFormat(wxT("%3.0f"));
	    y3_axis->SetLabelFormat(wxT("%3.0f"));
        m_Plot3->SetMargins(10, 10, 20, 20);
        m_Plot3->AddLayer( x3_axis );
        m_Plot3->AddLayer( y3_axis );

        mpFXYVector* vectorLayer31 = new mpFXYVector(_("Boat Speed"));
        vectorLayer31->SetData(vector_x2, vector_y3);
	    vectorLayer31->SetContinuity(true);
	    vectorpen.SetColour(*wxBLUE);
	    vectorLayer31->SetPen(vectorpen);
        m_Plot3->AddLayer(vectorLayer31, true);

        mpFXYVector* vectorLayer32 = new mpFXYVector(_("Wind Direction"));
        vectorLayer32->SetData(vector_x2, vector_y3);
	    vectorLayer32->SetContinuity(true);
	    vectorpen.SetColour(*wxRED);
	    vectorLayer32->SetPen(vectorpen);
        m_Plot3->AddLayer(vectorLayer32, true);

        m_Legend3 = new  mpInfoLegend(wxRect(200,20,100,40), wxTRANSPARENT_BRUSH);
        m_Legend3->SetName(_("Wind Direction & Speed"));
        m_Legend3->SetVisible(true);
        m_Plot3->AddLayer( m_Legend3);

      m_notebook->AddPage( m_Plot3, _("Other"), false );

	  bSizerNotebook->Add( m_notebook, 1, wxEXPAND | wxALL, 5 );


      SetSizer( bSizerNotebook );
      bSizerNotebook->Fit( this );
      Layout();
}

STE_Graph::~STE_Graph()
{
}
/*
STE_Graph::PointVect(std::vector<double> vectorx)
{
	double xcoord;
	for (unsigned int p = 0; p < 100; p++) {
		xcoord = ((double)p-50.0)*5.0;
		vectorx.push_back(xcoord);
		vectory.push_back(0.0001*pow(xcoord, 3));
	}
}
*/
//************* Point List Operations *****************************

wxSTE_PointListNode     *cur_node = NULL;
STE_Point               *cur_Point;

STE_PointListCtrl::STE_PointListCtrl( wxWindow* parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style ) :
        wxListCtrl( parent, id, pos, size, style )
{
    m_parent = parent;
    last_item = 0;
}

STE_PointListCtrl::~STE_PointListCtrl()
{
}

wxString STE_PointListCtrl::OnGetItemText(long item, long column) const
{
    wxString data;
    int index;

    if(cur_node == NULL)
        cur_node = m_pSTE_PointList->GetFirst();

    if (item != last_item) {
            if(item < last_item){
                index = 0;
                cur_node = m_pSTE_PointList->GetFirst();
            }
            else
                index = last_item;

            while (cur_node && ( index < item )){
                cur_node = cur_node->GetNext();
                index++;
            }
        last_item = item;
    }

    cur_Point = cur_node->GetData();
    switch (column)
    {
    case 0:
        data = cur_Point->UTS;
        break;
    case 1:
        data = cur_Point->SOG;
        break;
    case 2:
        data = cur_Point->COGT;
        break;
    case 3:
        data = cur_Point->TWA;
        break;
    case 4:
        data = cur_Point->TWS;
        break;
    case 5:
        data = cur_Point->RWA;
        break;
    case 6:
        data = cur_Point->Lat;
        break;
    case 7:
        data = cur_Point->Lon;
        break;

    }

    return data;
}

int STE_PointListCtrl::OnGetItemColumnImage( long item, long column ) const
{
    return -1;
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
    wxDateTime UTC;
    long m_date;
    long m_year;
    long m_month;
    long m_day;

    wxStringTokenizer tokenizer(UTS, _T(" "));
    wxString UTS_date = tokenizer.GetNextToken();
    if (UTS_date.Len() == 8)
    {
        UTS_date.ToLong(&m_date);
        m_year = m_date/10000;
        m_month = m_date/100 - m_year * 100;
        m_day = m_date - m_year * 10000 - m_month * 100;
    }
    else
    {
        m_year = 1970;
        m_month = 1;
        m_day = 1;
    }

    wxString UTS_time = tokenizer.GetNextToken();
    if (UTS_time.length() >= 7)
        UTC.ParseTime(UTS_time);

    UTC.SetDay(m_day);
    UTC.SetMonth(wxDateTime::Month(m_month - 1));
    UTC.SetYear(m_year);

    return UTC;
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
    TWD = _T("0.0");
    RWA = _T("0.0");
    TWA = _T("0.0");
    TWS = _T("0.0");
    RWS = _T("0.0");
    VMG_W = _T("0.0");
    BtHDG = _T("0.0");
    BtWatSpd = _T("0.0");
    WPLat = _T("0.0");
    WPLon = _T("0.0");
    WPRteCrse = _T("0.0");
    XTE = _T("0.0");
    BrngWP = _T("0.0");
    DistWP = _T("0.0");
    VMG_C = _T("0.0");
    Waypoint.Clear();
}

//************************************************************
// Math equations

double deg2rad(double deg)
{
    return ((90 - deg) * PI / 180.0);
}

double rad2deg(double rad)
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
    double RWA_R = RWA * PI/180.0;
    double TWS = sqrt(pow((RWS * sin(RWA_R)),2) + pow((RWS * cos(RWA_R)- SOG),2));
    return TWS;
}

double TWA(double RWS, double RWA, double SOG)
{
    double RWA_R = RWA*PI/180.0;
    double TWA_R = atan((RWS * sin(RWA_R))/(RWS * cos(RWA_R)-SOG));
    double TWA = TWA_R * 180.0/PI;
    if (TWA < 0)
        TWA = TWA + 180;
    if(RWA > 180.0)
        TWA = TWA + 180;       // atan only maps to 0->180 (pi/2 -> -pi/2)
    return TWA;
}

double RWS(double TWS, double TWA, double SOG)
{
    TWA = TWA*PI/180.0;
    double RWS_value = sqrt(pow((TWS * sin(TWA)),2) + pow((TWS * cos(TWA)+ SOG), 2));
    return RWS_value;
}

double RWA(double TWS, double TWA,double SOG)
{
    double TWA_R = TWA = TWA*PI/180.0;
    double RWA_R = atan((TWS * sin(TWA_R))/(TWS * cos(TWA_R)+ SOG));
    double RWA = RWA_R *180.0 / PI;
    if (RWA < 0)
        RWA = RWA + 180;
    if( TWA > 180 )
        RWA = RWA + 180;               // atan only maps to 0->180 (pi/2 -> -pi/2)
    return RWA;
}
