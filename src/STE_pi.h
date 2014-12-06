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
#include <wx/aui/aui.h>
#include "../../../include/ocpn_plugin.h"
#include "nmea0183/nmea0183.h"

#define STE_TOOL_POSITION -1          // Request default positioning of toolbar tool

//    Forward definitions

//class wxListCtrl;
class STE_Control;
class STE_Point;
//class STE_Analysis;
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

      bool Loadtxt_trt( void );
      bool make_trt_line(wxString sentence);
      bool NMEA_parse( wxString sentence);
      wxString build_trt_string(void);
      void SetStart( int position );
      void SetEnd( int position );
	  void clear_variables(void);

      STE_Point* Get_Record_Data(wxString* m_instr, STE_Point* m_Point);
      PlugIn_Waypoint* STE_to_PI_Waypoint( STE_Point *m_inpoint);
	  	  
      void Load_track();


//    The required override PlugIn Methods
      bool RenderGLOverlay(wxGLContext *pcontext, PlugIn_ViewPort *vp);
      bool RenderOverlay(wxDC &dc, PlugIn_ViewPort *vp);
      void Draw_Wind_Barb(wxPoint pp, double angle, double speed);

private:
      bool LoadConfig( void );
      bool SaveConfig( void );

      int               m_PlugIn_STE;

      wxFileConfig     *m_pconfig;
      wxAuiManager     *m_pauimgr;
      STE_Control       *m_pSTE_Control;

      wxWindow          *m_parent_window;
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
      wxButton          *bMakeFile;


private:
      void OnStartSliderUpdated( wxCommandEvent& event );
      void OnEndSliderUpdated( wxCommandEvent& event);
      void OnMakeFileButtonPush( wxCommandEvent& event );
     
      STE_pi           *m_pSTE;
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

    wxString UTS,           // "yyyymmdd hh:mm:ss Z"
            Lat,            // deg.decimal
            Lon,
            SOG,
            COGT,           // ALL Angular measurements are RADIANS
            COGM,
            Variation,
            Depth,
            CrseWind,
            RelWind,
            TrueWind,
            TWSpd,
            RWSpd,
            SpdParWind,
            BtHDG,
            BtWatSpd,
            WPLat,
            WPLon,
            WPRteCrse,
            XTE,
            BrngWP,
            DistWP,
            VMG,
            Waypoint;
      
};

WX_DECLARE_LIST(STE_Point, STE_PointList);

/*
class STE_Analysis
{
public:
    STE_Analysis(void);
    ~STE_Analysis(void);

};
*/

double compass_deg2rad(double deg);
double compass_rad2deg(double rad);
double local_distance (double lat1, double lon1, double lat2, double lon2);
double local_bearing (double lat1, double lon1, double lat2, double lon2);
double VTW(double VAW, double BAW, double SOG);
double BTW(double VAW, double BAW, double SOG);
double VAW(double VTW, double BTW, double SOG);
double BAW(double VTW, double BTW,double SOG);

#endif
