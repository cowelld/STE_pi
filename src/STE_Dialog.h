/*
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA  02110-1301, USA.

 
 */

#define _STE_Dialog_h_

#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
  #include "wx/wx.h"
#endif //precompiled headers


#include <wx/dialog.h>
#include <wx/timer.h>
#include <wx/panel.h>
#include <wx/listctrl.h>
#include <wx/notebook.h>

#define ID_LISTCTRL            1000

//class wxButton;
class wxListCtrl;

/***************************************************************************************************
  Data List Controls
****************************************************************************************************/

class STE_Dialog : public wxDialog
{
 //     DECLARE_EVENT_TABLE()
/*
      public:
        STE_Dialog(  wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos,
                const wxSize& size, long style );
        ~STE_Dialog();

        void CreateControls();

        void SetDialogTitle(const wxString & title);

        /// Should we show tooltips?
        static bool ShowToolTips();

        void STE_Dialog::OnEvtColDragEnd( wxListEvent& event );

        bool UpdateProperties(void);
	    wxString MakeTideInfo(int jx, time_t tm, int tz_selection, long LMT_Offset);
        bool SaveChanges(void);

        wxListCtrl  *m_wpList;
		wxListCtrl	*m_lcPoints;

        wxButton*     m_CancelButton;
        wxButton*     m_OKButton;

        bool          m_bStartNow;

        int         m_nSelected; // index of point selected in Properties dialog row
        int         m_tz_selection;

		protected:
        wxNotebook* m_notebook1;
        wxScrolledWindow* m_panelBasic;
        wxStaticText* m_stName;
        wxTextCtrl* m_tName;
        wxStaticText* m_stFrom;
        wxTextCtrl* m_tFrom;
        wxStaticText* m_stTo;
        wxTextCtrl* m_tTo;
        wxCheckBox* m_cbShow;
        wxStaticText* m_stColor;
        wxChoice* m_cColor;
        wxStaticText* m_stStyle;
        wxChoice* m_cStyle;
        wxStaticText* m_stWidth;
        wxChoice* m_cWidth;
        wxStaticText* m_stTotDistance;
        wxTextCtrl* m_tTotDistance;
        wxStaticText* m_stAvgSpeed;
        wxTextCtrl* m_tAvgSpeed;
        wxStaticText* m_stTimeEnroute;
        wxTextCtrl* m_tTimeEnroute;
        wxStaticText* m_stShowTime;
        wxRadioButton* m_rbShowTimeUTC;
        wxRadioButton* m_rbShowTimePC;
        wxRadioButton* m_rbShowTimeLocal;
//        OCPNTrackListCtrl *m_lcPoints;
        wxScrolledWindow* m_panelAdvanced;
        wxStaticText* m_stDescription;
        wxTextCtrl* m_tDescription;
        wxScrolledWindow* m_scrolledWindowLinks;
//        wxHyperlinkCtrl* m_hyperlink1;
        wxMenu* m_menuLink;
        wxButton* m_buttonAddLink;
//        wxToggleButton* m_toggleBtnEdit;
        wxStaticText* m_staticTextEditEnabled;
        wxStdDialogButtonSizer* m_sdbBtmBtnsSizer;
        wxButton* m_sdbBtmBtnsSizerOK;
        wxButton* m_sdbBtmBtnsSizerCancel;
        wxStaticBoxSizer* sbSizerLinks;
        wxBoxSizer* bSizerLinks;
    
        wxButton* m_sdbBtmBtnsSizerPrint;
        wxButton* m_sdbBtmBtnsSizerSplit;
        wxButton* m_sdbBtmBtnsSizerExtend;
        wxButton* m_sdbBtmBtnsSizerToRoute;
        wxButton* m_sdbBtmBtnsSizerExport;

        // Virtual event handlers, overide them in your derived class
        void OnCancelBtnClick( wxCommandEvent& event );
        void OnOKBtnClick( wxCommandEvent& event );
        void OnPrintBtnClick( wxCommandEvent& event );
        void OnSplitBtnClick( wxCommandEvent& event );
        void OnExtendBtnClick( wxCommandEvent& event );
        void OnToRouteBtnClick( wxCommandEvent& event );
        void OnExportBtnClick( wxCommandEvent& event );
        void OnTrackPropCopyTxtClick( wxCommandEvent& event );
        void OnTrackPropListClick( wxListEvent& event );
        void OnTrackPropRightClick( wxListEvent &event );
        void OnTrackPropMenuSelected( wxCommandEvent &event );
        void OnDeleteLink( wxCommandEvent& event );
        void OnEditLink( wxCommandEvent& event );
        void OnAddLink( wxCommandEvent& event );
        void OnEditLinkToggle( wxCommandEvent& event );
        void OnShowTimeTZ( wxCommandEvent& event );
*/
};