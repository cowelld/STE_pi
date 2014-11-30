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

    ---
    Copyright (C) 2010, Anders Lund <anders@alweb.dk>
 */

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "STE_pi.h"
#include "STE_Dialog.h"
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/progdlg.h>
#include <wx/clipbrd.h>

#include <iostream>


/*STE_Dialog::STE_Dialog( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
    long wstyle = style;
#ifdef __WXOSX__
    wstyle |= wxSTAY_ON_TOP;
#endif
//    
    wxBoxSizer* bSizerMain;
    bSizerMain = new wxBoxSizer( wxVERTICAL );
    
    wxBoxSizer* bSizerBasic;
    bSizerBasic = new wxBoxSizer( wxVERTICAL );
//************************************************************************
    wxBoxSizer* bSizerName;
    bSizerName = new wxBoxSizer( wxHORIZONTAL );

    m_stName = new wxStaticText( m_panelBasic, wxID_ANY, _("Name"), wxDefaultPosition, wxDefaultSize, 0 );
    m_stName->Wrap( -1 );
    bSizerName->Add( m_stName, 0, wxALL, 5 );
    
    m_tName = new wxTextCtrl( m_panelBasic, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    bSizerName->Add( m_tName, 1, 0, 5 );

    bSizerBasic->Add( bSizerName, 0, wxALL|wxEXPAND, 5 );
//*************************************************************************    
    wxBoxSizer* bSizerFromTo;
    bSizerFromTo = new wxBoxSizer( wxHORIZONTAL );
    
    m_stFrom = new wxStaticText( m_panelBasic, wxID_ANY, _("From"), wxDefaultPosition, wxDefaultSize, 0 );
    m_stFrom->Wrap( -1 );
    bSizerFromTo->Add( m_stFrom, 0, wxALL, 5 );
    
    m_tFrom = new wxTextCtrl( m_panelBasic, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    bSizerFromTo->Add( m_tFrom, 1, 0, 5 );
    
    m_stTo = new wxStaticText( m_panelBasic, wxID_ANY, _("To"), wxDefaultPosition, wxDefaultSize, 0 );
    m_stTo->Wrap( -1 );
    bSizerFromTo->Add( m_stTo, 0, wxALL, 5 );
    
    m_tTo = new wxTextCtrl( m_panelBasic, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    bSizerFromTo->Add( m_tTo, 1, 0, 5 );   
    
    bSizerMain->Add( bSizerFromTo, 0, wxALL|wxEXPAND, 5 );
//**************************************************************************
	/* 
    wxStaticBoxSizer* sbSizerParams;
    sbSizerParams = new wxStaticBoxSizer( new wxStaticBox( m_panelBasic, wxID_ANY, _("Display parameters") ), wxHORIZONTAL );
    
    m_cbShow = new wxCheckBox( m_panelBasic, wxID_ANY, _("Show on chart"), wxDefaultPosition, wxDefaultSize, 0 );
    sbSizerParams->Add( m_cbShow, 0, wxALL, 5 );
    
    m_stColor = new wxStaticText( m_panelBasic, wxID_ANY, _("Color"), wxDefaultPosition, wxDefaultSize, 0 );
    m_stColor->Wrap( -1 );
    sbSizerParams->Add( m_stColor, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
    
    wxString m_cColorChoices[] = { _("Default color"), _("Black"), _("Dark Red"), _("Dark Green"),
            _("Dark Yellow"), _("Dark Blue"), _("Dark Magenta"), _("Dark Cyan"),
            _("Light Gray"), _("Dark Gray"), _("Red"), _("Green"), _("Yellow"), _("Blue"),
            _("Magenta"), _("Cyan"), _("White") };
    int m_cColorNChoices = sizeof( m_cColorChoices ) / sizeof(wxString);
    m_cColor = new wxChoice( m_panelBasic, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_cColorNChoices, m_cColorChoices, 0 );
    m_cColor->SetSelection( 0 );
    sbSizerParams->Add( m_cColor, 1, 0, 5 );
    
    m_stStyle = new wxStaticText( m_panelBasic, wxID_ANY, _("Style"), wxDefaultPosition, wxDefaultSize, 0 );
    m_stStyle->Wrap( -1 );
    sbSizerParams->Add( m_stStyle, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
    
    wxString m_cStyleChoices[] = { _("Default"), _("Solid"), _("Dot"), _("Long dash"), _("Short dash"), _("Dot dash") };
    int m_cStyleNChoices = sizeof( m_cStyleChoices ) / sizeof(wxString);
    m_cStyle = new wxChoice( m_panelBasic, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_cStyleNChoices, m_cStyleChoices, 0 );
    m_cStyle->SetSelection( 0 );
    sbSizerParams->Add( m_cStyle, 1, 0, 5 );
    
    m_stWidth = new wxStaticText( m_panelBasic, wxID_ANY, _("Width"), wxDefaultPosition, wxDefaultSize, 0 );
    m_stWidth->Wrap( -1 );
    sbSizerParams->Add( m_stWidth, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
    
    wxString m_cWidthChoices[] = { _("Default"), _("1 pixel"), _("2 pixels"), _("3 pixels"),
            _("4 pixels"), _("5 pixels"), _("6 pixels"), _("7 pixels"), _("8 pixels"),
            _("9 pixels"), _("10 pixels") };
    int m_cWidthNChoices = sizeof( m_cWidthChoices ) / sizeof(wxString);
    m_cWidth = new wxChoice( m_panelBasic, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_cWidthNChoices, m_cWidthChoices, 0 );
    m_cWidth->SetSelection( 0 );
    sbSizerParams->Add( m_cWidth, 1, 0, 5 );
    
    bSizerMain->Add( sbSizerParams, 0, wxALL|wxEXPAND, 5 );
    */
//************************** Stats **************************************
/*	wxStaticBoxSizer* sbSizerStats;
    sbSizerStats = new wxStaticBoxSizer( new wxStaticBox( m_panelBasic, wxID_ANY, _("Statistics") ), wxVERTICAL );
    
    wxBoxSizer* bSizerStats;
    bSizerStats = new wxBoxSizer( wxHORIZONTAL );
    
    m_stTotDistance = new wxStaticText( m_panelBasic, wxID_ANY, _("Total distance"), wxDefaultPosition, wxDefaultSize, 0 );
    m_stTotDistance->Wrap( -1 );
    bSizerStats->Add( m_stTotDistance, 0, wxALL, 5 );
    
    m_tTotDistance = new wxTextCtrl( m_panelBasic, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
    bSizerStats->Add( m_tTotDistance, 1, 0, 5 );
    
    m_stAvgSpeed = new wxStaticText( m_panelBasic, wxID_ANY, _("Avg. speed"), wxDefaultPosition, wxDefaultSize, 0 );
    m_stAvgSpeed->Wrap( -1 );
    bSizerStats->Add( m_stAvgSpeed, 0, wxALL, 5 );
    
    m_tAvgSpeed = new wxTextCtrl( m_panelBasic, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
    bSizerStats->Add( m_tAvgSpeed, 1, 0, 5 );
    
    m_stTimeEnroute = new wxStaticText( m_panelBasic, wxID_ANY, _("Time enroute"), wxDefaultPosition, wxDefaultSize, 0 );
    m_stTimeEnroute->Wrap( -1 );
    bSizerStats->Add( m_stTimeEnroute, 0, wxALL, 5 );
    
    m_tTimeEnroute = new wxTextCtrl( m_panelBasic, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
    bSizerStats->Add( m_tTimeEnroute, 2, 0, 5 );
       
    sbSizerStats->Add( bSizerStats, 0, wxEXPAND, 5 );
        
    bSizerBasic->Add( sbSizerStats, 0, wxALL|wxEXPAND, 5 );

//******************** Points ********************************************   
    wxStaticBoxSizer* sbSizerPoints;
    sbSizerPoints = new wxStaticBoxSizer( new wxStaticBox( m_panelBasic, wxID_ANY, _("Recorded points") ), wxVERTICAL );
    /*
    wxBoxSizer* bSizerShowTime;
    bSizerShowTime = new wxBoxSizer( wxHORIZONTAL );
    
    m_stShowTime = new wxStaticText( m_panelBasic, wxID_ANY, _("Time shown as"), wxDefaultPosition, wxDefaultSize, 0 );
    m_stShowTime->Wrap( -1 );
    bSizerShowTime->Add( m_stShowTime, 0, wxALL, 5 );
    
    m_rbShowTimeUTC = new wxRadioButton( m_panelBasic, wxID_ANY, _("UTC"), wxDefaultPosition, wxDefaultSize, 0 );
    bSizerShowTime->Add( m_rbShowTimeUTC, 0, 0, 5 );
    
    m_rbShowTimePC = new wxRadioButton( m_panelBasic, wxID_ANY, _("Local @ PC"), wxDefaultPosition, wxDefaultSize, 0 );
    bSizerShowTime->Add( m_rbShowTimePC, 0, 0, 5 );

    m_rbShowTimeLocal = new wxRadioButton( m_panelBasic, wxID_ANY, _("LMT @ Track Start"), wxDefaultPosition, wxDefaultSize, 0 );
    bSizerShowTime->Add( m_rbShowTimeLocal, 0, 0, 5 );
    
    m_rbShowTimePC->SetValue(true);
    
    sbSizerPoints->Add( bSizerShowTime, 0, wxEXPAND, 5 );
    */
/*    m_lcPoints = new wxListCtrl( m_panelBasic, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_HRULES | wxLC_VRULES | wxLC_EDIT_LABELS | wxLC_VIRTUAL );

    m_lcPoints->InsertColumn( 0, _("Leg"), wxLIST_FORMAT_LEFT, 45 );
    m_lcPoints->InsertColumn( 2, _("Distance"), wxLIST_FORMAT_RIGHT, 70 );
    m_lcPoints->InsertColumn( 3, _("Bearing"), wxLIST_FORMAT_LEFT, 70 );
    m_lcPoints->InsertColumn( 4, _("Latitude"), wxLIST_FORMAT_LEFT, 85 );
    m_lcPoints->InsertColumn( 5, _("Longitude"), wxLIST_FORMAT_LEFT, 90 );
    m_lcPoints->InsertColumn( 6, _("Timestamp"), wxLIST_FORMAT_LEFT, 135 );
    m_lcPoints->InsertColumn( 7, _("Speed"), wxLIST_FORMAT_CENTER, 100 );

    sbSizerPoints->Add( m_lcPoints, 1, wxALL|wxEXPAND, 5 );
    
    bSizerBasic->Add( sbSizerPoints, 1, wxALL|wxEXPAND, 5 );
//********************Basic Panel ****************************************    
    m_panelBasic->SetSizer( bSizerBasic );
    m_panelBasic->Layout();
    bSizerBasic->Fit( m_panelBasic );
    m_notebook1->AddPage( m_panelBasic, _("Basic"), true );

//******************** Advanced Panel **************************************
    m_panelAdvanced = new wxScrolledWindow( m_notebook1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    
    wxBoxSizer* bSizerAdvanced;
    bSizerAdvanced = new wxBoxSizer( wxVERTICAL );
    
    m_stDescription = new wxStaticText( m_panelAdvanced, wxID_ANY, _("Description"), wxDefaultPosition, wxDefaultSize, 0 );
    m_stDescription->Wrap( -1 );
    bSizerAdvanced->Add( m_stDescription, 0, wxALL, 5 );
    
    m_tDescription = new wxTextCtrl( m_panelAdvanced, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
    bSizerAdvanced->Add( m_tDescription, 1, wxALL|wxEXPAND, 5 );
/*    
    sbSizerLinks = new wxStaticBoxSizer( new wxStaticBox( m_panelAdvanced, wxID_ANY, _("Links") ), wxVERTICAL );
    
    m_scrolledWindowLinks = new wxScrolledWindow( m_panelAdvanced, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
    m_scrolledWindowLinks->SetScrollRate( 5, 5 );
    /*
    bSizerLinks = new wxBoxSizer( wxVERTICAL );
    
    m_hyperlink1 = new wxHyperlinkCtrl( m_scrolledWindowLinks, wxID_ANY, _("wxFB Website"), wxT("http://www.wxformbuilder.org"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
    m_menuLink = new wxMenu();
    wxMenuItem* m_menuItemEdit;
    m_menuItemEdit = new wxMenuItem( m_menuLink, wxID_ANY, wxString( _("Edit") ) , wxEmptyString, wxITEM_NORMAL );
    m_menuLink->Append( m_menuItemEdit );
    
    wxMenuItem* m_menuItemAdd;
    m_menuItemAdd = new wxMenuItem( m_menuLink, wxID_ANY, wxString( _("Add new") ) , wxEmptyString, wxITEM_NORMAL );
    m_menuLink->Append( m_menuItemAdd );
    
    wxMenuItem* m_menuItemDelete;
    m_menuItemDelete = new wxMenuItem( m_menuLink, wxID_ANY, wxString( _("Delete") ) , wxEmptyString, wxITEM_NORMAL );
    m_menuLink->Append( m_menuItemDelete );
    
    m_hyperlink1->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( TrackPropDlg::m_hyperlink1OnContextMenu ), NULL, this ); 
    
    bSizerLinks->Add( m_hyperlink1, 0, wxALL, 5 );
    
    
    m_scrolledWindowLinks->SetSizer( bSizerLinks );
    m_scrolledWindowLinks->Layout();
    bSizerLinks->Fit( m_scrolledWindowLinks );
    sbSizerLinks->Add( m_scrolledWindowLinks, 1, wxEXPAND | wxALL, 5 );
    
    wxBoxSizer* bSizer27;
    bSizer27 = new wxBoxSizer( wxHORIZONTAL );
    
    m_buttonAddLink = new wxButton( m_panelAdvanced, wxID_ANY, _("Add"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    bSizer27->Add( m_buttonAddLink, 0, wxALL, 5 );
    
    m_toggleBtnEdit = new wxToggleButton( m_panelAdvanced, wxID_ANY, _("Edit"), wxDefaultPosition, wxDefaultSize, 0 );
    bSizer27->Add( m_toggleBtnEdit, 0, wxALL, 5 );
    
    m_staticTextEditEnabled = new wxStaticText( m_panelAdvanced, wxID_ANY, _("Links are opened in the default browser."), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextEditEnabled->Wrap( -1 );
    bSizer27->Add( m_staticTextEditEnabled, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
    
    
    sbSizerLinks->Add( bSizer27, 0, wxEXPAND, 5 );
    
    
    bSizerAdvanced->Add( sbSizerLinks, 1, wxEXPAND, 5 );
    
 */   
/*    m_panelAdvanced->SetSizer( bSizerAdvanced );
    m_panelAdvanced->Layout();
    bSizerAdvanced->Fit( m_panelAdvanced );
    m_notebook1->AddPage( m_panelAdvanced, _("Advanced"), false );
    
    bSizerMain->Add( m_notebook1, 1, wxEXPAND | wxALL, 5 );
    
    m_sdbBtmBtnsSizer = new wxStdDialogButtonSizer();
    m_sdbBtmBtnsSizerOK = new wxButton( this, wxID_OK );
    m_sdbBtmBtnsSizer->AddButton( m_sdbBtmBtnsSizerOK );
    m_sdbBtmBtnsSizerCancel = new wxButton( this, wxID_CANCEL );
    m_sdbBtmBtnsSizer->AddButton( m_sdbBtmBtnsSizerCancel );

    m_sdbBtmBtnsSizerPrint = new wxButton( this, wxID_ANY, _("Print"), wxDefaultPosition, wxDefaultSize );
    m_sdbBtmBtnsSizer->Add( m_sdbBtmBtnsSizerPrint );
    m_sdbBtmBtnsSizerSplit = new wxButton( this, wxID_ANY, _("Split"), wxDefaultPosition, wxDefaultSize );
    m_sdbBtmBtnsSizer->Add( m_sdbBtmBtnsSizerSplit );
    m_sdbBtmBtnsSizerExtend = new wxButton( this, wxID_ANY, _("Extend"), wxDefaultPosition, wxDefaultSize );
    m_sdbBtmBtnsSizer->Add( m_sdbBtmBtnsSizerExtend );
    m_sdbBtmBtnsSizerToRoute = new wxButton( this, wxID_ANY, _("To route"), wxDefaultPosition, wxDefaultSize );
    m_sdbBtmBtnsSizer->Add( m_sdbBtmBtnsSizerToRoute );
    m_sdbBtmBtnsSizerExport = new wxButton( this, wxID_ANY, _("Export"), wxDefaultPosition, wxDefaultSize );
    m_sdbBtmBtnsSizer->Add( m_sdbBtmBtnsSizerExport );

    m_sdbBtmBtnsSizer->Realize();

    bSizerMain->Add( m_sdbBtmBtnsSizer, 0, wxALL|wxEXPAND, 5 );
    
    //Make it look nice and add the needed non-standard buttons
    int w1, w2, h;
    ((wxWindowBase *)m_stName)->GetSize( &w1, &h );
    ((wxWindowBase *)m_stFrom)->GetSize( &w2, &h );
    ((wxWindowBase *)m_stName)->SetMinSize( wxSize(wxMax(w1, w2), h) );
    ((wxWindowBase *)m_stFrom)->SetMinSize( wxSize(wxMax(w1, w2), h) );
    
    this->SetSizer( bSizerMain );
    this->Layout();
    
    this->Centre( wxBOTH );
    /*
    // Connect Events
    m_sdbBtmBtnsSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TrackPropDlg::OnCancelBtnClick ), NULL, this );
    m_sdbBtmBtnsSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TrackPropDlg::OnOKBtnClick ), NULL, this );
    m_sdbBtmBtnsSizerPrint->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TrackPropDlg::OnPrintBtnClick ), NULL, this );
    m_sdbBtmBtnsSizerSplit->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TrackPropDlg::OnSplitBtnClick ), NULL, this );
    m_sdbBtmBtnsSizerExtend->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TrackPropDlg::OnExtendBtnClick ), NULL, this );
    m_sdbBtmBtnsSizerToRoute->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TrackPropDlg::OnToRouteBtnClick ), NULL, this );
    m_sdbBtmBtnsSizerExport->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TrackPropDlg::OnExportBtnClick ), NULL, this );
    m_lcPoints->Connect(wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( TrackPropDlg::OnTrackPropListClick ), NULL, this );
    Connect( wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK, wxListEventHandler(TrackPropDlg::OnTrackPropRightClick), NULL, this );
    Connect( wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(TrackPropDlg::OnTrackPropMenuSelected), NULL, this );
    
    Connect( m_menuItemDelete->GetId(), wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler( TrackPropDlg::OnDeleteLink ) );
    Connect( m_menuItemEdit->GetId(), wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler( TrackPropDlg::OnEditLink ) );
    Connect( m_menuItemAdd->GetId(), wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler( TrackPropDlg::OnAddLink ) );
    m_buttonAddLink->Connect( wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler( TrackPropDlg::OnAddLink ), NULL, this );
    m_toggleBtnEdit->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED,
            wxCommandEventHandler( TrackPropDlg::OnEditLinkToggle ), NULL, this );
    
    m_rbShowTimeUTC->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( TrackPropDlg::OnShowTimeTZ), NULL, this );
    m_rbShowTimePC->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( TrackPropDlg::OnShowTimeTZ), NULL, this );
    m_rbShowTimeLocal->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( TrackPropDlg::OnShowTimeTZ), NULL, this );
        
    m_pLinkProp = new LinkPropImpl( this );
    m_pMyLinkList = NULL;   
}

STE_Dialog::~STE_Dialog()
{
};

void STE_Dialog::OnEvtColDragEnd( wxListEvent& event )
{
    m_wpList->Refresh();
} */