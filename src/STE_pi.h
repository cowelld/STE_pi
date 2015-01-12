/******************************************************************************
 * $Id: STE_pi.h, v0.2 11/15/14 
 *
 * Project:  OpenCPN
 * Purpose:  STE Plugin
 * Author:   Dave Cowell
 *                 Plagiarized from Jean-Eudes Onfray
 *
 ***************************************************************************                                                               *
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

#ifndef _STEPI_H_
#define _STEPI_H_

#include "wx/wxprec.h"
#include "wx/plotctrl/plotctrl.h"

#ifndef  WX_PRECOMP
  #include "wx/wx.h"
#endif //precompiled headers

#define     PLUGIN_VERSION_MAJOR    0
#define     PLUGIN_VERSION_MINOR    1

#define     MY_API_VERSION_MAJOR    1
#define     MY_API_VERSION_MINOR    11

#define ID_LISTCTRL            1000
#define ID_MAKE_FILE           1001


#ifndef PI
      #define PI        3.1415926535897931160E0      /* pi */
#endif

//#include <wx/fileconf.h>
//#include <wx/filepicker.h>
//#include <wx/file.h>
#include <wx/listctrl.h>
#include <wx/aui/aui.h>
#include <wx/notebook.h>
#include "../../../include/ocpn_plugin.h"
#include "nmea0183/nmea0183.h"
#include "mathplot.h"

#define STE_TOOL_POSITION -1          // Request default positioning of toolbar tool

//    Forward definitions

//class wxListCtrl;
class STE_Control;
class STE_Point;
class STE_PointListCtrl;
class STE_Graph;
class STE_Analysis;

class PlugIn_Waypoint;

//----------------------------------------------------------------------------------------------------------
//    The PlugIn Class Definition
//----------------------------------------------------------------------------------------------------------

class STE_pi : public opencpn_plugin_111
{
public:
      STE_pi( void *ppimgr );
      ~STE_pi(void);
//    The required PlugIn Methods
      int Init( void );
      bool DeInit( void );

      int GetAPIVersionMajor();
      int GetAPIVersionMinor();
      int GetPlugInVersionMajor();
      int GetPlugInVersionMinor();

      wxBitmap *GetPlugInBitmap();
      wxString GetCommonName();
      wxString GetShortDescription();
      wxString GetLongDescription();

      int GetToolbarToolCount( void );
      void OnToolbarToolCallback( int id );
      void ShowPreferencesDialog(wxWindow* parent);
//      void SetColorScheme( PI_ColorScheme cs );
      void SetCursorLatLon(double lat, double lon);

      bool Loadtxt_trt( void );
      bool make_trt_line(wxString sentence);
      bool NMEA_parse( wxString sentence);
      wxString build_trt_string(STE_Point *Point);
      void SetStart( int position );
      void SetEnd( int position );
	  void clear_variables(void);

      STE_Point* Get_Record_Data(wxString* m_instr, STE_Point* m_Point);
      PlugIn_Waypoint* STE_to_PI_Waypoint( STE_Point *m_inpoint);
      STE_Analysis      *m_pAnalysis;

      void Load_track();

//    The required override PlugIn Methods
      bool RenderGLOverlay(wxGLContext *pcontext, PlugIn_ViewPort *vp);
      bool RenderOverlay(wxDC &dc, PlugIn_ViewPort *vp);
      void Draw_Wind_Barb(wxPoint pp, double TWD, double speed);
      void DrawCircle(wxPoint pp, float r, int num_segments);

private:
      bool LoadConfig( void );
      bool SaveConfig( void );

      int               m_PlugIn_STE;

      wxWindow          *m_parent_window;
      wxFileConfig     *m_pconfig;
      wxAuiManager     *m_pauimgr;
      STE_Control       *m_pSTE_Control;

      STE_Graph       *m_pGraph;

      NMEA0183          m_NMEA0183;
      wxRadioBox        *pFileType;
      wxRadioBox        *pTrackInterval;
      
};

//----------------------------------------------------------------------------------------------------------
//    STE Controls Dialog Specification
//----------------------------------------------------------------------------------------------------------
class STE_Control : public wxWindow
{
public:
      STE_Control( wxWindow *pparent, wxWindowID id, STE_pi *STE, int start, int end, int range, wxString file_name );
      ~STE_Control();

      wxBoxSizer        *topsizer;
      wxStaticText      *Start_text;
      wxSlider          *m_start_slider;
      wxStaticText      *End_text;
      wxSlider          *m_end_slider;

        wxBoxSizer        *bSizerName;
        wxStaticText      *m_stName;
        wxTextCtrl        *m_tName;

        wxBoxSizer        *bSizerFromTo;
        wxStaticText      *m_stFrom;
        wxTextCtrl        *m_tFrom;
        wxStaticText      *m_stTo;
        wxTextCtrl        *m_tTo;

      wxStaticBoxSizer* sbSizerPoints;
      STE_PointListCtrl   *m_lcPoints;
      wxButton          *bMakeFile;
      wxButton          *bValidate_wind;


private:
      void OnStartSliderUpdated( wxCommandEvent& event );
      void OnEndSliderUpdated( wxCommandEvent& event);
      void OnMakeFileButtonPush( wxCommandEvent& event );
      void OnTrackPropListClick( wxListEvent& event );
      void OnValidate_windButtonPush(wxCommandEvent& event);
     
      STE_pi           *m_pSTE;
};

//----------------------------------------------------------------------------------------------------------
//    STE Graph Specification
//----------------------------------------------------------------------------------------------------------
class STE_Graph : public wxWindow
{
public:
      STE_Graph( wxWindow *pparent, wxWindowID id, STE_pi *STE, wxPoint Position, wxSize Size );
      ~STE_Graph();


      wxBoxSizer      *bSizerNotebook;
      wxNotebook      *m_notebook;
      wxPanel         *m_panel1;
 //     wxBoxSizer      *m_panel1_sizer;
      wxPanel         *m_panel2;
      wxBoxSizer      *m_panel2_sizer;
      wxPanel         *m_panel3;
      wxBoxSizer      *m_panel3_sizer;

      mpWindow    *m_Plot1;
      mpInfoLegend *m_Legend1; 
      mpWindow    *m_Plot2;
      mpInfoLegend *m_Legend2;
      mpWindow    *m_Plot3;
      mpInfoLegend *m_Legend3;

      STE_pi           *m_pSTE;

    std::vector<double> vector_x1;
    std::vector<double> vector_x2;
    std::vector<double> vector_y1;
    std::vector<double> vector_y2;
    std::vector<double> vector_y3;
};

class STE_Point
{
public:
      STE_Point();
      ~STE_Point();

      double GetLat();
      double GetLon();
      wxDateTime GetUTS();
      void Clear();

    wxString UTS,           // "yyyymmdd hh:mm:ss NF"
            Lat,            // deg.decimal
            Lon,
            SOG,
            COGT,           // ALL Angular measurements are RADIANS in OLD
            COGM,           // Format in NEW FORMAT they're degrees (0-359)
            Variation,
            Depth,
            TWD,
            RWA,
            TWA,
            TWS,
            RWS,
            VMG_W,
            BtHDG,
            BtWatSpd,
            WPLat,
            WPLon,
            WPRteCrse,
            XTE,
            BrngWP,
            DistWP,
            VMG_C,
            Waypoint;
      
};

WX_DECLARE_LIST(STE_Point, STE_PointList);

//***************** Point List Scrolling ************************
class STE_PointListCtrl: public wxListCtrl
{
    public:
        STE_PointListCtrl(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style);
        ~STE_PointListCtrl();

        wxString OnGetItemText(long item, long column) const;
        int OnGetItemColumnImage( long item, long column ) const;

        STE_PointList *m_pSTE_PointList;
};


class STE_Analysis
{
    public:
        STE_Analysis();
        ~STE_Analysis();

        double max_TWS;
        long max_TWD;
        double max_SOG;
};


double deg2rad(double deg);
double rad2deg(double rad);
double local_distance (double lat1, double lon1, double lat2, double lon2);
double local_bearing (double lat1, double lon1, double lat2, double lon2);
double TWS(double RWS, double RWA, double SOG);
double TWA(double RWS, double RWA, double SOG);
double RWS(double TWS, double TWA, double SOG);
double RWA(double TWS, double TWA,double SOG);
double UTC_to_seconds(wxDateTime UTC_date);

#endif
