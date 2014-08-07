#include "StdAfx.h"
#include "UIlist.h"

namespace DuiLib {

/////////////////////////////////////////////////////////////////////////////////////
//
//

CListUI::CListUI() : m_pCallback(NULL), m_bScrollSelect(false), m_iCurSel(-1), m_iExpandedItem(-1)
{
    m_pList = new CListBodyUI(this);
    m_pHeader = new CListHeaderUI;

    Add(m_pHeader);
    CVerticalLayoutUI::Add(m_pList);

    m_ListInfo.nColumns = 0;
    m_ListInfo.nFont = -1;
    m_ListInfo.uTextStyle = DT_VCENTER; // m_uTextStyle(DT_VCENTER | DT_END_ELLIPSIS)
    m_ListInfo.dwTextColor = 0xFF000000;
    m_ListInfo.dwBkColor = 0;
    m_ListInfo.bAlternateBk = false;
    m_ListInfo.dwSelectedTextColor = 0xFF000000;
    m_ListInfo.dwSelectedBkColor = 0xFFC1E3FF;
    m_ListInfo.dwHotTextColor = 0xFF000000;
    m_ListInfo.dwHotBkColor = 0xFFE9F5FF;
    m_ListInfo.dwDisabledTextColor = 0xFFCCCCCC;
    m_ListInfo.dwDisabledBkColor = 0xFFFFFFFF;
    m_ListInfo.dwLineColor = 0;
    m_ListInfo.bShowHtml = false;
    m_ListInfo.bMultiExpandable = false;
    ::ZeroMemory(&m_ListInfo.rcTextPadding, sizeof(m_ListInfo.rcTextPadding));
    ::ZeroMemory(&m_ListInfo.rcColumn, sizeof(m_ListInfo.rcColumn));

#ifdef UI_BUILD_FOR_DESIGNER
	SetUsedForPitchUpon(true);
#endif
}

LPCTSTR CListUI::GetClass() const
{
    return kListUIClassName;
}

UINT CListUI::GetControlFlags() const
{
#ifdef UI_BUILD_FOR_DESIGNER
	return (IsUsedForPitchUpon() && (m_pManager != NULL) && m_pManager->IsUsedAsUIDesigner())?UIFLAG_SETCURSOR:UIFLAG_TABSTOP;
#else
	return UIFLAG_TABSTOP;
#endif
}

LPVOID CListUI::GetInterface(LPCTSTR pstrName)
{
	if( _tcsicmp(pstrName, kListUIInterfaceName) == 0 ) return static_cast<CListUI*>(this);
    if( _tcsicmp(pstrName, kIListUIInterfaceName) == 0 ) return static_cast<IListUI*>(this);
    if( _tcsicmp(pstrName, kIListOwnerUIInterfaceName) == 0 ) return static_cast<IListOwnerUI*>(this);
    return CVerticalLayoutUI::GetInterface(pstrName);
}

CControlUI* CListUI::GetItemAt(int iIndex) const
{
    return m_pList->GetItemAt(iIndex);
}

int CListUI::GetItemIndex(CControlUI* pControl) const
{
    if( pControl->GetInterface(kListHeaderUIInterfaceName) != NULL ) return CVerticalLayoutUI::GetItemIndex(pControl);
    // We also need to recognize header sub-items
    if( _tcsstr(pControl->GetClass(), kListHeaderItemUIClassName) != NULL ) return m_pHeader->GetItemIndex(pControl);

    return m_pList->GetItemIndex(pControl);
}

bool CListUI::SetItemIndex(CControlUI* pControl, int iIndex)
{
    if( pControl->GetInterface(kListHeaderUIInterfaceName) != NULL ) return CVerticalLayoutUI::SetItemIndex(pControl, iIndex);
    // We also need to recognize header sub-items
    if( _tcsstr(pControl->GetClass(), kListHeaderItemUIClassName) != NULL ) return m_pHeader->SetItemIndex(pControl, iIndex);

    int iOrginIndex = m_pList->GetItemIndex(pControl);
    if( iOrginIndex == -1 ) return false;
    if( iOrginIndex == iIndex ) return true;

    IListItemUI* pSelectedListItem = NULL;
    if( m_iCurSel >= 0 ) pSelectedListItem = 
        static_cast<IListItemUI*>(GetItemAt(m_iCurSel)->GetInterface(kIListItemUIInterfaceName));
    if( !m_pList->SetItemIndex(pControl, iIndex) ) return false;
    int iMinIndex = min(iOrginIndex, iIndex);
    int iMaxIndex = max(iOrginIndex, iIndex);
    for(int i = iMinIndex; i < iMaxIndex + 1; ++i) {
        CControlUI* p = m_pList->GetItemAt(i);
        IListItemUI* pListItem = static_cast<IListItemUI*>(p->GetInterface(kIListItemUIInterfaceName));
        if( pListItem != NULL ) {
            pListItem->SetIndex(i);
        }
    }
    if( m_iCurSel >= 0 && pSelectedListItem != NULL ) m_iCurSel = pSelectedListItem->GetIndex();
    return true;
}

int CListUI::GetCount() const
{
    return m_pList->GetCount();
}

bool CListUI::Add(CControlUI* pControl)
{
    // Override the Add() method so we can add items specifically to
    // the intended widgets. Headers are assumed to be
    // answer the correct interface so we can add multiple list headers.
    if( pControl->GetInterface(kListHeaderUIInterfaceName) != NULL ) {
        if( m_pHeader != pControl && m_pHeader->GetCount() == 0 ) {
            CVerticalLayoutUI::Remove(m_pHeader);
			if (!m_bAutoDestroy)
				delete m_pHeader;
            m_pHeader = static_cast<CListHeaderUI*>(pControl);
        }
        m_ListInfo.nColumns = MIN(m_pHeader->GetCount(), UILIST_MAX_COLUMNS);
        return CVerticalLayoutUI::AddAt(pControl, 0);
    }
    // We also need to recognize header sub-items
    if( _tcsstr(pControl->GetClass(), kListHeaderItemUIClassName) != NULL ) {
        bool ret = m_pHeader->Add(pControl);
        m_ListInfo.nColumns = MIN(m_pHeader->GetCount(), UILIST_MAX_COLUMNS);
        return ret;
    }
    // The list items should know about us
    IListItemUI* pListItem = static_cast<IListItemUI*>(pControl->GetInterface(kIListItemUIInterfaceName));
    if( pListItem != NULL ) {
        pListItem->SetOwner(this);
        pListItem->SetIndex(GetCount());
    }
    return m_pList->Add(pControl);
}

bool CListUI::AddAt(CControlUI* pControl, int iIndex)
{
    // Override the AddAt() method so we can add items specifically to
    // the intended widgets. Headers and are assumed to be
    // answer the correct interface so we can add multiple list headers.
    if( pControl->GetInterface(kListHeaderUIInterfaceName) != NULL ) {
        if( m_pHeader != pControl && m_pHeader->GetCount() == 0 ) {
            CVerticalLayoutUI::Remove(m_pHeader);
			if (!m_bAutoDestroy)
				delete m_pHeader;			
            m_pHeader = static_cast<CListHeaderUI*>(pControl);
        }
        m_ListInfo.nColumns = MIN(m_pHeader->GetCount(), UILIST_MAX_COLUMNS);
        return CVerticalLayoutUI::AddAt(pControl, 0);
    }
    // We also need to recognize header sub-items
    if( _tcsstr(pControl->GetClass(), kListHeaderItemUIClassName) != NULL ) {
        bool ret = m_pHeader->AddAt(pControl, iIndex);
        m_ListInfo.nColumns = MIN(m_pHeader->GetCount(), UILIST_MAX_COLUMNS);
        return ret;
    }
    if (!m_pList->AddAt(pControl, iIndex)) return false;

    // The list items should know about us
    IListItemUI* pListItem = static_cast<IListItemUI*>(pControl->GetInterface(kIListItemUIInterfaceName));
    if( pListItem != NULL ) {
        pListItem->SetOwner(this);
        pListItem->SetIndex(iIndex);
    }

    for(int i = iIndex + 1; i < m_pList->GetCount(); ++i) {
        CControlUI* p = m_pList->GetItemAt(i);
        pListItem = static_cast<IListItemUI*>(p->GetInterface(kIListItemUIInterfaceName));
        if( pListItem != NULL ) {
            pListItem->SetIndex(i);
        }
    }
    if( m_iCurSel >= iIndex ) m_iCurSel += 1;
    return true;
}

bool CListUI::Remove(CControlUI* pControl)
{
	if( pControl->GetInterface(kListHeaderUIInterfaceName) != NULL ) return CVerticalLayoutUI::Remove(pControl);
	// We also need to recognize header sub-items
	if( _tcsstr(pControl->GetClass(), kListHeaderItemUIClassName) != NULL ) return m_pHeader->Remove(pControl);

	int iIndex = m_pList->GetItemIndex(pControl);
	if (iIndex == -1) return false;

	if (!m_pList->RemoveAt(iIndex)) return false;

	for(int i = iIndex; i < m_pList->GetCount(); ++i) {
		CControlUI* p = m_pList->GetItemAt(i);
		IListItemUI* pListItem = static_cast<IListItemUI*>(p->GetInterface(kIListItemUIInterfaceName));
		if( pListItem != NULL ) {
			pListItem->SetIndex(i);
		}
	}

	if( iIndex == m_iCurSel && m_iCurSel >= 0 ) {
		int iSel = m_iCurSel;
		m_iCurSel = -1;
		SelectItem(FindSelectable(iSel, false));
	}
	else if( iIndex < m_iCurSel ) m_iCurSel -= 1;
	return true;
}

bool CListUI::RemoveAt(int iIndex)
{
    if (!m_pList->RemoveAt(iIndex)) return false;

    for(int i = iIndex; i < m_pList->GetCount(); ++i) {
        CControlUI* p = m_pList->GetItemAt(i);
        IListItemUI* pListItem = static_cast<IListItemUI*>(p->GetInterface(kIListItemUIInterfaceName));
        if( pListItem != NULL ) pListItem->SetIndex(i);
    }

    if( iIndex == m_iCurSel && m_iCurSel >= 0 ) {
        int iSel = m_iCurSel;
        m_iCurSel = -1;
        SelectItem(FindSelectable(iSel, false));
    }
    else if( iIndex < m_iCurSel ) m_iCurSel -= 1;
    return true;
}

void CListUI::RemoveAll()
{
    m_iCurSel = -1;
    m_iExpandedItem = -1;
    m_pList->RemoveAll();
}

void CListUI::SetPos(RECT rc)
{
    CVerticalLayoutUI::SetPos(rc);
    if( m_pHeader == NULL ) return;
    // Determine general list information and the size of header columns
    m_ListInfo.nColumns = MIN(m_pHeader->GetCount(), UILIST_MAX_COLUMNS);
    // The header/columns may or may not be visible at runtime. In either case
    // we should determine the correct dimensions...

    if( !m_pHeader->IsVisible() ) {
        for( int it = 0; it < m_pHeader->GetCount(); it++ ) {
            static_cast<CControlUI*>(m_pHeader->GetItemAt(it))->SetInternVisible(true);
        }
        m_pHeader->SetPos(CRect(rc.left, 0, rc.right, 0));
    }
    int iOffset = m_pList->GetScrollPos().cx;
    for( int i = 0; i < m_ListInfo.nColumns; i++ ) {
        CControlUI* pControl = static_cast<CControlUI*>(m_pHeader->GetItemAt(i));
        if( !pControl->IsVisible() ) continue;
        if( pControl->IsFloat() ) continue;

        RECT rcPos = pControl->GetPos();
        if( iOffset > 0 ) {
            rcPos.left -= iOffset;
            rcPos.right -= iOffset;
            pControl->SetPos(rcPos);
        }
        m_ListInfo.rcColumn[i] = pControl->GetPos();
    }
    if( !m_pHeader->IsVisible() ) {
        for( int it = 0; it < m_pHeader->GetCount(); it++ ) {
            static_cast<CControlUI*>(m_pHeader->GetItemAt(it))->SetInternVisible(false);
        }
    }
}

void CListUI::DoEvent(TEventUI& event)
{
#ifdef UI_BUILD_FOR_DESIGNER
	if (IsUsedForPitchUpon() && (m_pManager != NULL) && m_pManager->IsUsedAsUIDesigner() && ((GetKeyState(VK_CONTROL) < 0) || (GetKeyState(VK_SHIFT) < 0)))
	{
		CVerticalLayoutUI::DoEvent(event);
		return;
	}
#endif

    if( !IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND ) {
        if( m_pParent != NULL ) m_pParent->DoEvent(event);
        else CVerticalLayoutUI::DoEvent(event);
        return;
    }

    if( event.Type == UIEVENT_SETFOCUS ) 
    {
        m_bFocused = true;
        return;
    }
    if( event.Type == UIEVENT_KILLFOCUS ) 
    {
        m_bFocused = false;
        return;
    }

    switch( event.Type ) {
    case UIEVENT_KEYDOWN:
        switch( event.chKey ) {
        case VK_UP:
            SelectItem(FindSelectable(m_iCurSel - 1, false), true);
            return;
        case VK_DOWN:
            SelectItem(FindSelectable(m_iCurSel + 1, true), true);
            return;
        case VK_PRIOR:
            PageUp();
            return;
        case VK_NEXT:
            PageDown();
            return;
        case VK_HOME:
            SelectItem(FindSelectable(0, false), true);
            return;
        case VK_END:
            SelectItem(FindSelectable(GetCount() - 1, true), true);
            return;
        case VK_RETURN:
            if( m_iCurSel != -1 ) GetItemAt(m_iCurSel)->Activate();
            return;
            }
        break;
    case UIEVENT_SCROLLWHEEL:
        {
            switch( LOWORD(event.wParam) ) {
            case SB_LINEUP:
                if( m_bScrollSelect ) SelectItem(FindSelectable(m_iCurSel - 1, false), true);
                else LineUp();
                return;
            case SB_LINEDOWN:
                if( m_bScrollSelect ) SelectItem(FindSelectable(m_iCurSel + 1, true), true);
                else LineDown();
                return;
            }
        }
        break;
    }
    CVerticalLayoutUI::DoEvent(event);
}

CListHeaderUI* CListUI::GetHeader() const
{
    return m_pHeader;
}

CContainerUI* CListUI::GetList() const
{
    return m_pList;
}

bool CListUI::GetScrollSelect()
{
    return m_bScrollSelect;
}

void CListUI::SetScrollSelect(bool bScrollSelect)
{
    m_bScrollSelect = bScrollSelect;
}

int CListUI::GetCurSel() const
{
    return m_iCurSel;
}

bool CListUI::SelectItem(int iIndex, bool bTakeFocus, bool bSendNofitied)
{
    if( iIndex == m_iCurSel ) return true;

    int iOldSel = m_iCurSel;
    // We should first unselect the currently selected item
    if( m_iCurSel >= 0 ) {
        CControlUI* pControl = GetItemAt(m_iCurSel);
        if( pControl != NULL) {
            IListItemUI* pListItem = static_cast<IListItemUI*>(pControl->GetInterface(kIListItemUIInterfaceName));
            if( pListItem != NULL ) pListItem->Select(false);
        }

        m_iCurSel = -1;
    }
    if( iIndex < 0 ) return false;

    CControlUI* pControl = GetItemAt(iIndex);
    if( pControl == NULL ) return false;
    if( !pControl->IsVisible() ) return false;
    if( !pControl->IsEnabled() ) return false;

    IListItemUI* pListItem = static_cast<IListItemUI*>(pControl->GetInterface(kIListItemUIInterfaceName));
    if( pListItem == NULL ) return false;
    m_iCurSel = iIndex;
    if( !pListItem->Select(true) ) {
        m_iCurSel = -1;
        return false;
    }
    EnsureVisible(m_iCurSel);
    if( bTakeFocus ) pControl->SetFocus();
    if(bSendNofitied && ( m_pManager != NULL )) {
        m_pManager->SendNotify(this, kItemSelect, m_iCurSel, iOldSel);
    }

    return true;
}

bool CListUI::Activate()
{
	// 对普通的list，鼠标单击响应itemclick 双击或者enter键需要响应itemactivate
	// 而对Combox，如果通过上下键或者鼠标滚轮选中某个item时不会接收到消息，只有当按enter键捉着鼠标单击或者双击时，需响应itemselect
	return true;
}

TListInfoUI* CListUI::GetListInfo()
{
    return &m_ListInfo;
}

int CListUI::GetChildPadding() const
{
    return m_pList->GetChildPadding();
}

void CListUI::SetChildPadding(int iPadding)
{
    m_pList->SetChildPadding(iPadding);
}

void CListUI::SetItemFont(int index)
{
    m_ListInfo.nFont = index;
    NeedUpdate();
}

void CListUI::SetItemTextStyle(UINT uStyle)
{
    m_ListInfo.uTextStyle = uStyle;
    NeedUpdate();
}

void CListUI::SetItemTextPadding(RECT rc)
{
    m_ListInfo.rcTextPadding = rc;
    NeedUpdate();
}

RECT CListUI::GetItemTextPadding() const
{
	return m_ListInfo.rcTextPadding;
}

void CListUI::SetItemTextColor(DWORD dwTextColor)
{
    m_ListInfo.dwTextColor = dwTextColor;
    Invalidate();
}

void CListUI::SetItemBkColor(DWORD dwBkColor)
{
    m_ListInfo.dwBkColor = dwBkColor;
    Invalidate();
}

void CListUI::SetItemBkImage(LPCTSTR pStrImage)
{
    m_ListInfo.sBkImage = pStrImage;
    Invalidate();
}

void CListUI::SetAlternateBk(bool bAlternateBk)
{
    m_ListInfo.bAlternateBk = bAlternateBk;
    Invalidate();
}


DWORD CListUI::GetItemTextColor() const
{
	return m_ListInfo.dwTextColor;
}

DWORD CListUI::GetItemBkColor() const
{
	return m_ListInfo.dwBkColor;
}

LPCTSTR CListUI::GetItemBkImage() const
{
	return m_ListInfo.sBkImage;
}

bool CListUI::IsAlternateBk() const
{
    return m_ListInfo.bAlternateBk;
}


void CListUI::SetSelectedItemTextColor(DWORD dwTextColor)
{
    m_ListInfo.dwSelectedTextColor = dwTextColor;
    Invalidate();
}

void CListUI::SetSelectedItemBkColor(DWORD dwBkColor)
{
    m_ListInfo.dwSelectedBkColor = dwBkColor;
    Invalidate();
}

void CListUI::SetSelectedItemImage(LPCTSTR pStrImage)
{
    m_ListInfo.sSelectedImage = pStrImage;
    Invalidate();
}

DWORD CListUI::GetSelectedItemTextColor() const
{
	return m_ListInfo.dwSelectedTextColor;
}

DWORD CListUI::GetSelectedItemBkColor() const
{
	return m_ListInfo.dwSelectedBkColor;
}

LPCTSTR CListUI::GetSelectedItemImage() const
{
	return m_ListInfo.sSelectedImage;
}

void CListUI::SetHotItemTextColor(DWORD dwTextColor)
{
    m_ListInfo.dwHotTextColor = dwTextColor;
    Invalidate();
}

void CListUI::SetHotItemBkColor(DWORD dwBkColor)
{
    m_ListInfo.dwHotBkColor = dwBkColor;
    Invalidate();
}

void CListUI::SetHotItemImage(LPCTSTR pStrImage)
{
    m_ListInfo.sHotImage = pStrImage;
    Invalidate();
}

DWORD CListUI::GetHotItemTextColor() const
{
	return m_ListInfo.dwHotTextColor;
}
DWORD CListUI::GetHotItemBkColor() const
{
	return m_ListInfo.dwHotBkColor;
}

LPCTSTR CListUI::GetHotItemImage() const
{
	return m_ListInfo.sHotImage;
}

void CListUI::SetDisabledItemTextColor(DWORD dwTextColor)
{
    m_ListInfo.dwDisabledTextColor = dwTextColor;
    Invalidate();
}

void CListUI::SetDisabledItemBkColor(DWORD dwBkColor)
{
    m_ListInfo.dwDisabledBkColor = dwBkColor;
    Invalidate();
}

void CListUI::SetDisabledItemImage(LPCTSTR pStrImage)
{
    m_ListInfo.sDisabledImage = pStrImage;
    Invalidate();
}

DWORD CListUI::GetDisabledItemTextColor() const
{
	return m_ListInfo.dwDisabledTextColor;
}

DWORD CListUI::GetDisabledItemBkColor() const
{
	return m_ListInfo.dwDisabledBkColor;
}

LPCTSTR CListUI::GetDisabledItemImage() const
{
	return m_ListInfo.sDisabledImage;
}

DWORD CListUI::GetItemLineColor() const
{
	return m_ListInfo.dwLineColor;
}

void CListUI::SetItemLineColor(DWORD dwLineColor)
{
    m_ListInfo.dwLineColor = dwLineColor;
    Invalidate();
}

bool CListUI::IsItemShowHtml()
{
    return m_ListInfo.bShowHtml;
}

void CListUI::SetItemShowHtml(bool bShowHtml)
{
    if( m_ListInfo.bShowHtml == bShowHtml ) return;

    m_ListInfo.bShowHtml = bShowHtml;
    NeedUpdate();
}

void CListUI::SetMultiExpanding(bool bMultiExpandable)
{
    m_ListInfo.bMultiExpandable = bMultiExpandable;
}

bool CListUI::ExpandItem(int iIndex, bool bExpand /*= true*/)
{
    if( m_iExpandedItem >= 0 && !m_ListInfo.bMultiExpandable) {
        CControlUI* pControl = GetItemAt(m_iExpandedItem);
        if( pControl != NULL ) {
            IListItemUI* pItem = static_cast<IListItemUI*>(pControl->GetInterface(kIListItemUIInterfaceName));
            if( pItem != NULL ) pItem->Expand(false);
        }
        m_iExpandedItem = -1;
    }
    if( bExpand ) {
        CControlUI* pControl = GetItemAt(iIndex);
        if( pControl == NULL ) return false;
        if( !pControl->IsVisible() ) return false;
        IListItemUI* pItem = static_cast<IListItemUI*>(pControl->GetInterface(kIListItemUIInterfaceName));
        if( pItem == NULL ) return false;
        m_iExpandedItem = iIndex;
        if( !pItem->Expand(true) ) {
            m_iExpandedItem = -1;
            return false;
        }
    }
    NeedUpdate();
    return true;
}

int CListUI::GetExpandedItem() const
{
    return m_iExpandedItem;
}

void CListUI::EnsureVisible(int iIndex)
{
    if( m_iCurSel < 0 ) return;
    RECT rcItem = m_pList->GetItemAt(iIndex)->GetPos();
    RECT rcList = m_pList->GetPos();
    RECT rcListInset = m_pList->GetInset();

    rcList.left += rcListInset.left;
    rcList.top += rcListInset.top;
    rcList.right -= rcListInset.right;
    rcList.bottom -= rcListInset.bottom;

    CScrollBarUI* pHorizontalScrollBar = m_pList->GetHorizontalScrollBar();
    if( pHorizontalScrollBar && pHorizontalScrollBar->IsVisible() ) rcList.bottom -= pHorizontalScrollBar->GetFixedHeight();

    int iPos = m_pList->GetScrollPos().cy;
    if( rcItem.top >= rcList.top && rcItem.bottom < rcList.bottom ) return;
    int dx = 0;
    if( rcItem.top < rcList.top ) dx = rcItem.top - rcList.top;
    if( rcItem.bottom > rcList.bottom ) dx = rcItem.bottom - rcList.bottom;
    Scroll(0, dx);
}

void CListUI::Scroll(int dx, int dy)
{
    if( dx == 0 && dy == 0 ) return;
    SIZE sz = m_pList->GetScrollPos();
    m_pList->SetScrollPos(CSize(sz.cx + dx, sz.cy + dy));
}

void CListUI::SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue)
{
    if( _tcsicmp(pstrName, _T("header")) == 0 ) GetHeader()->SetVisible(_tcsicmp(pstrValue, _T("hidden")) != 0);
    else if( _tcsicmp(pstrName, _T("headerbkimage")) == 0 ) GetHeader()->SetBkImage(pstrValue);
    else if( _tcsicmp(pstrName, _T("scrollselect")) == 0 ) SetScrollSelect(_tcsicmp(pstrValue, _T("true")) == 0);
    else if( _tcsicmp(pstrName, _T("multiexpanding")) == 0 ) SetMultiExpanding(_tcsicmp(pstrValue, _T("true")) == 0);
    else if( _tcsicmp(pstrName, _T("itemfont")) == 0 ) m_ListInfo.nFont = _ttoi(pstrValue);
    else if( _tcsicmp(pstrName, _T("itemalign")) == 0 ) {
        if( _tcsstr(pstrValue, _T("left")) != NULL ) {
            m_ListInfo.uTextStyle &= ~(DT_CENTER | DT_RIGHT);
            m_ListInfo.uTextStyle |= DT_LEFT;
        }
        if( _tcsstr(pstrValue, _T("center")) != NULL ) {
            m_ListInfo.uTextStyle &= ~(DT_LEFT | DT_RIGHT);
            m_ListInfo.uTextStyle |= DT_CENTER;
        }
        if( _tcsstr(pstrValue, _T("right")) != NULL ) {
            m_ListInfo.uTextStyle &= ~(DT_LEFT | DT_CENTER);
            m_ListInfo.uTextStyle |= DT_RIGHT;
        }
    }
    else if( _tcsicmp(pstrName, _T("itemendellipsis")) == 0 ) {
        if( _tcsicmp(pstrValue, _T("true")) == 0 ) m_ListInfo.uTextStyle |= DT_END_ELLIPSIS;
        else m_ListInfo.uTextStyle &= ~DT_END_ELLIPSIS;
    }    
    if( _tcsicmp(pstrName, _T("itemtextpadding")) == 0 ) {
        RECT rcTextPadding = { 0 };
        LPTSTR pstr = NULL;
        rcTextPadding.left = _tcstol(pstrValue, &pstr, 10);  ASSERT(pstr);    
        rcTextPadding.top = _tcstol(pstr + 1, &pstr, 10);    ASSERT(pstr);    
        rcTextPadding.right = _tcstol(pstr + 1, &pstr, 10);  ASSERT(pstr);    
        rcTextPadding.bottom = _tcstol(pstr + 1, &pstr, 10); ASSERT(pstr);    
        SetItemTextPadding(rcTextPadding);
    }
    else if( _tcsicmp(pstrName, _T("itemtextcolor")) == 0 ) {
        if( *pstrValue == _T('#')) pstrValue = ::CharNext(pstrValue);
        LPTSTR pstr = NULL;
        DWORD clrColor = _tcstoul(pstrValue, &pstr, 16);
        SetItemTextColor(clrColor);
    }
    else if( _tcsicmp(pstrName, _T("itembkcolor")) == 0 ) {
        if( *pstrValue == _T('#')) pstrValue = ::CharNext(pstrValue);
        LPTSTR pstr = NULL;
        DWORD clrColor = _tcstoul(pstrValue, &pstr, 16);
        SetItemBkColor(clrColor);
    }
    else if( _tcsicmp(pstrName, _T("itembkimage")) == 0 ) SetItemBkImage(pstrValue);
    else if( _tcsicmp(pstrName, _T("itemaltbk")) == 0 ) SetAlternateBk(_tcsicmp(pstrValue, _T("true")) == 0);
    else if( _tcsicmp(pstrName, _T("itemselectedtextcolor")) == 0 ) {
        if( *pstrValue == _T('#')) pstrValue = ::CharNext(pstrValue);
        LPTSTR pstr = NULL;
        DWORD clrColor = _tcstoul(pstrValue, &pstr, 16);
        SetSelectedItemTextColor(clrColor);
    }
    else if( _tcsicmp(pstrName, _T("itemselectedbkcolor")) == 0 ) {
        if( *pstrValue == _T('#')) pstrValue = ::CharNext(pstrValue);
        LPTSTR pstr = NULL;
        DWORD clrColor = _tcstoul(pstrValue, &pstr, 16);
        SetSelectedItemBkColor(clrColor);
    }
    else if( _tcsicmp(pstrName, _T("itemselectedimage")) == 0 ) SetSelectedItemImage(pstrValue);
    else if( _tcsicmp(pstrName, _T("itemhottextcolor")) == 0 ) {
        if( *pstrValue == _T('#')) pstrValue = ::CharNext(pstrValue);
        LPTSTR pstr = NULL;
        DWORD clrColor = _tcstoul(pstrValue, &pstr, 16);
        SetHotItemTextColor(clrColor);
    }
    else if( _tcsicmp(pstrName, _T("itemhotbkcolor")) == 0 ) {
        if( *pstrValue == _T('#')) pstrValue = ::CharNext(pstrValue);
        LPTSTR pstr = NULL;
        DWORD clrColor = _tcstoul(pstrValue, &pstr, 16);
        SetHotItemBkColor(clrColor);
    }
    else if( _tcsicmp(pstrName, _T("itemhotimage")) == 0 ) SetHotItemImage(pstrValue);
    else if( _tcsicmp(pstrName, _T("itemdisabledtextcolor")) == 0 ) {
        if( *pstrValue == _T('#')) pstrValue = ::CharNext(pstrValue);
        LPTSTR pstr = NULL;
        DWORD clrColor = _tcstoul(pstrValue, &pstr, 16);
        SetDisabledItemTextColor(clrColor);
    }
    else if( _tcsicmp(pstrName, _T("itemdisabledbkcolor")) == 0 ) {
        if( *pstrValue == _T('#')) pstrValue = ::CharNext(pstrValue);
        LPTSTR pstr = NULL;
        DWORD clrColor = _tcstoul(pstrValue, &pstr, 16);
        SetDisabledItemBkColor(clrColor);
    }
    else if( _tcsicmp(pstrName, _T("itemdisabledimage")) == 0 ) SetDisabledItemImage(pstrValue);
    else if( _tcsicmp(pstrName, _T("itemlinecolor")) == 0 ) {
        if( *pstrValue == _T('#')) pstrValue = ::CharNext(pstrValue);
        LPTSTR pstr = NULL;
        DWORD clrColor = _tcstoul(pstrValue, &pstr, 16);
        SetItemLineColor(clrColor);
    }
    else if( _tcsicmp(pstrName, _T("itemshowhtml")) == 0 ) SetItemShowHtml(_tcsicmp(pstrValue, _T("true")) == 0);
    else CVerticalLayoutUI::SetAttribute(pstrName, pstrValue);
}

IListCallbackUI* CListUI::GetTextCallback() const
{
    return m_pCallback;
}

void CListUI::SetTextCallback(IListCallbackUI* pCallback)
{
    m_pCallback = pCallback;
}

SIZE CListUI::GetScrollPos() const
{
    return m_pList->GetScrollPos();
}

SIZE CListUI::GetScrollRange() const
{
    return m_pList->GetScrollRange();
}

void CListUI::SetScrollPos(SIZE szPos)
{
    m_pList->SetScrollPos(szPos);
}

void CListUI::LineUp()
{
    m_pList->LineUp();
}

void CListUI::LineDown()
{
    m_pList->LineDown();
}

void CListUI::PageUp()
{
    m_pList->PageUp();
}

void CListUI::PageDown()
{
    m_pList->PageDown();
}

void CListUI::HomeUp()
{
    m_pList->HomeUp();
}

void CListUI::EndDown()
{
    m_pList->EndDown();
}

void CListUI::LineLeft()
{
    m_pList->LineLeft();
}

void CListUI::LineRight()
{
    m_pList->LineRight();
}

void CListUI::PageLeft()
{
    m_pList->PageLeft();
}

void CListUI::PageRight()
{
    m_pList->PageRight();
}

void CListUI::HomeLeft()
{
    m_pList->HomeLeft();
}

void CListUI::EndRight()
{
    m_pList->EndRight();
}

void CListUI::EnableScrollBar(bool bEnableVertical, bool bEnableHorizontal)
{
    m_pList->EnableScrollBar(bEnableVertical, bEnableHorizontal);
}

CScrollBarUI* CListUI::GetVerticalScrollBar() const
{
    return m_pList->GetVerticalScrollBar();
}

CScrollBarUI* CListUI::GetHorizontalScrollBar() const
{
    return m_pList->GetHorizontalScrollBar();
}

/////////////////////////////////////////////////////////////////////////////////////
//
//


CListBodyUI::CListBodyUI(CListUI* pOwner) : m_pOwner(pOwner)
{
    ASSERT(m_pOwner);
    SetInset(CRect(0,0,0,0));

#ifdef UI_BUILD_FOR_DESIGNER
	SetUsedForPitchUpon(false);
#endif
}

UINT CListBodyUI::GetControlFlags() const
{
#ifdef UI_BUILD_FOR_DESIGNER
	return ((m_pManager != NULL) && m_pManager->IsUsedAsUIDesigner())?UIFLAG_SETCURSOR:0;
#else
	return 0;
#endif
}

void CListBodyUI::SetScrollPos(SIZE szPos)
{
    int cx = 0;
    int cy = 0;
    if( m_pVerticalScrollBar && m_pVerticalScrollBar->IsVisible() ) {
        int iLastScrollPos = m_pVerticalScrollBar->GetScrollPos();
        m_pVerticalScrollBar->SetScrollPos(szPos.cy);
        cy = m_pVerticalScrollBar->GetScrollPos() - iLastScrollPos;
    }

    if( m_pHorizontalScrollBar && m_pHorizontalScrollBar->IsVisible() ) {
        int iLastScrollPos = m_pHorizontalScrollBar->GetScrollPos();
        m_pHorizontalScrollBar->SetScrollPos(szPos.cx);
        cx = m_pHorizontalScrollBar->GetScrollPos() - iLastScrollPos;
    }

    if( cx == 0 && cy == 0 ) return;

#pragma region TAG_List中listItem的大小设置放到ListHeader大小设置之后
    //RECT rcPos;
    //for( int it2 = 0; it2 < m_items.GetSize(); it2++ ) {
    //    CControlUI* pControl = static_cast<CControlUI*>(m_items[it2]);
    //    if( !pControl->IsVisible() ) continue;
    //    if( pControl->IsFloat() ) continue;

    //    rcPos = pControl->GetPos();
    //    rcPos.left -= cx;
    //    rcPos.right -= cx;
    //    rcPos.top -= cy;
    //    rcPos.bottom -= cy;
    //    pControl->SetPos(rcPos);
    //}

    //Invalidate();
#pragma endregion

    if( cx != 0 && m_pOwner ) {
        CListHeaderUI* pHeader = m_pOwner->GetHeader();
        if( pHeader == NULL ) return;
        TListInfoUI* pInfo = m_pOwner->GetListInfo();
        pInfo->nColumns = MIN(pHeader->GetCount(), UILIST_MAX_COLUMNS);

        if( !pHeader->IsVisible() ) {
            for( int it = 0; it < pHeader->GetCount(); it++ ) {
                static_cast<CControlUI*>(pHeader->GetItemAt(it))->SetInternVisible(true);
            }
        }
        for( int i = 0; i < pInfo->nColumns; i++ ) {
            CControlUI* pControl = static_cast<CControlUI*>(pHeader->GetItemAt(i));
            if( !pControl->IsVisible() ) continue;
            if( pControl->IsFloat() ) continue;

            RECT rcPos = pControl->GetPos();
            rcPos.left -= cx;
            rcPos.right -= cx;
            pControl->SetPos(rcPos);
            pInfo->rcColumn[i] = pControl->GetPos();
        }
        if( !pHeader->IsVisible() ) {
            for( int it = 0; it < pHeader->GetCount(); it++ ) {
                static_cast<CControlUI*>(pHeader->GetItemAt(it))->SetInternVisible(false);
            }
        }
    }

#pragma region TAG_List中listItem的大小设置放到此处
    RECT rcPos;
    for( int it2 = 0; it2 < m_items.GetSize(); it2++ ) {
        CControlUI* pControl = static_cast<CControlUI*>(m_items[it2]);
        if( !pControl->IsVisible() ) continue;
        if( pControl->IsFloat() ) continue;

        rcPos = pControl->GetPos();
        rcPos.left -= cx;
        rcPos.right -= cx;
        rcPos.top -= cy;
        rcPos.bottom -= cy;
        pControl->SetPos(rcPos);
    }

    Invalidate();
#pragma endregion
}

void CListBodyUI::SetPos(RECT rc)
{
    CControlUI::SetPos(rc);
    rc = m_rcItem;

    // Adjust for inset
    rc.left += m_rcInset.left;
    rc.top += m_rcInset.top;
    rc.right -= m_rcInset.right;
    rc.bottom -= m_rcInset.bottom;
    if( m_pVerticalScrollBar && m_pVerticalScrollBar->IsVisible() ) rc.right -= m_pVerticalScrollBar->GetFixedWidth();
    if( m_pHorizontalScrollBar && m_pHorizontalScrollBar->IsVisible() ) rc.bottom -= m_pHorizontalScrollBar->GetFixedHeight();

    // Determine the minimum size
    SIZE szAvailable = { rc.right - rc.left, rc.bottom - rc.top };
    if( m_pHorizontalScrollBar && m_pHorizontalScrollBar->IsVisible() ) 
        szAvailable.cx += m_pHorizontalScrollBar->GetScrollRange();

    int cxNeeded = 0;
    int nAdjustables = 0;
    int cyFixed = 0;
    int nEstimateNum = 0;
    for( int it1 = 0; it1 < m_items.GetSize(); it1++ ) {
        CControlUI* pControl = static_cast<CControlUI*>(m_items[it1]);
        if( !pControl->IsVisible() ) continue;
        if( pControl->IsFloat() ) continue;
        SIZE sz = pControl->EstimateSize(szAvailable);
        if( sz.cy == 0 ) {
            nAdjustables++;
        }
        else {
            if( sz.cy < pControl->GetMinHeight() ) sz.cy = pControl->GetMinHeight();
            if( sz.cy > pControl->GetMaxHeight() ) sz.cy = pControl->GetMaxHeight();
        }
        cyFixed += sz.cy + pControl->GetPadding().top + pControl->GetPadding().bottom;

        RECT rcPadding = pControl->GetPadding();
        sz.cx = MAX(sz.cx, 0);
        if( sz.cx < pControl->GetMinWidth() ) sz.cx = pControl->GetMinWidth();
        if( sz.cx > pControl->GetMaxWidth() ) sz.cx = pControl->GetMaxWidth();
        cxNeeded = MAX(cxNeeded, sz.cx);
        nEstimateNum++;
    }
    cyFixed += (nEstimateNum - 1) * m_iChildPadding;

    if( m_pOwner ) {
        CListHeaderUI* pHeader = m_pOwner->GetHeader();
        if( pHeader != NULL && pHeader->GetCount() > 0 ) {
            cxNeeded = MAX(0, pHeader->EstimateSize(CSize(rc.right - rc.left, rc.bottom - rc.top)).cx);
        }
    }

    // Place elements
    int cyNeeded = 0;
    int cyExpand = 0;
    if( nAdjustables > 0 ) cyExpand = MAX(0, (szAvailable.cy - cyFixed) / nAdjustables);
    // Position the elements
    SIZE szRemaining = szAvailable;
    int iPosY = rc.top;
    if( m_pVerticalScrollBar && m_pVerticalScrollBar->IsVisible() ) {
        iPosY -= m_pVerticalScrollBar->GetScrollPos();
    }
    int iPosX = rc.left;
    if( m_pHorizontalScrollBar && m_pHorizontalScrollBar->IsVisible() ) {
        iPosX -= m_pHorizontalScrollBar->GetScrollPos();
    }
    int iAdjustable = 0;
    int cyFixedRemaining = cyFixed;
    for( int it2 = 0; it2 < m_items.GetSize(); it2++ ) {
        CControlUI* pControl = static_cast<CControlUI*>(m_items[it2]);
        if( !pControl->IsVisible() ) continue;
        if( pControl->IsFloat() ) {
            SetFloatPos(it2);
            continue;
        }

        RECT rcPadding = pControl->GetPadding();
        szRemaining.cy -= rcPadding.top;
        SIZE sz = pControl->EstimateSize(szRemaining);
        if( sz.cy == 0 ) {
            iAdjustable++;
            sz.cy = cyExpand;
            // Distribute remaining to last element (usually round-off left-overs)
            if( iAdjustable == nAdjustables ) {
                sz.cy = MAX(0, szRemaining.cy - rcPadding.bottom - cyFixedRemaining);
            } 
            if( sz.cy < pControl->GetMinHeight() ) sz.cy = pControl->GetMinHeight();
            if( sz.cy > pControl->GetMaxHeight() ) sz.cy = pControl->GetMaxHeight();
        }
        else {
            if( sz.cy < pControl->GetMinHeight() ) sz.cy = pControl->GetMinHeight();
            if( sz.cy > pControl->GetMaxHeight() ) sz.cy = pControl->GetMaxHeight();
            cyFixedRemaining -= sz.cy;
        }

        sz.cx = MAX(cxNeeded, szAvailable.cx - rcPadding.left - rcPadding.right);

        if( sz.cx < pControl->GetMinWidth() ) sz.cx = pControl->GetMinWidth();
        if( sz.cx > pControl->GetMaxWidth() ) sz.cx = pControl->GetMaxWidth();

        RECT rcCtrl = { iPosX + rcPadding.left, iPosY + rcPadding.top, iPosX + rcPadding.left + sz.cx, iPosY + sz.cy + rcPadding.top + rcPadding.bottom };
        pControl->SetPos(rcCtrl);

        iPosY += sz.cy + m_iChildPadding + rcPadding.top + rcPadding.bottom;
        cyNeeded += sz.cy + rcPadding.top + rcPadding.bottom;
        szRemaining.cy -= sz.cy + m_iChildPadding + rcPadding.bottom;
    }
    cyNeeded += (nEstimateNum - 1) * m_iChildPadding;

    if( m_pHorizontalScrollBar != NULL ) {
        if( cxNeeded > rc.right - rc.left ) {
            if( m_pHorizontalScrollBar->IsVisible() ) {
                m_pHorizontalScrollBar->SetScrollRange(cxNeeded - (rc.right - rc.left));
            }
            else {
                m_pHorizontalScrollBar->SetVisible(true);
                m_pHorizontalScrollBar->SetScrollRange(cxNeeded - (rc.right - rc.left));
                m_pHorizontalScrollBar->SetScrollPos(0);
                rc.bottom -= m_pHorizontalScrollBar->GetFixedHeight();
            }
        }
        else {
            if( m_pHorizontalScrollBar->IsVisible() ) {
                m_pHorizontalScrollBar->SetVisible(false);
                m_pHorizontalScrollBar->SetScrollRange(0);
                m_pHorizontalScrollBar->SetScrollPos(0);
                rc.bottom += m_pHorizontalScrollBar->GetFixedHeight();
            }
        }
    }

    // Process the scrollbar
    ProcessScrollBar(rc, cxNeeded, cyNeeded);
}

void CListBodyUI::DoEvent(TEventUI& event)
{
#ifdef UI_BUILD_FOR_DESIGNER
	if (m_pOwner != NULL)
	{
		if (((m_pManager != NULL) && m_pManager->IsUsedAsUIDesigner() && ((GetKeyState(VK_CONTROL) < 0) || (GetKeyState(VK_SHIFT) < 0))) &&
			(( event.Type == UIEVENT_SETCURSOR )	||
			 ( event.Type == UIEVENT_BUTTONDOWN )	||
			 ( event.Type == UIEVENT_BUTTONUP )		||
			 (m_pOwner->IsPitchUpon() && ( event.Type == UIEVENT_MOUSEMOVE ))))
		{
			event.pSender = m_pOwner;
			m_pOwner->DoEvent(event);
			return;
		}
	}
#endif
    if( !IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND ) {
        if( m_pOwner != NULL ) m_pOwner->DoEvent(event);
        else CControlUI::DoEvent(event);
        return;
    }

    if( m_pOwner != NULL ) m_pOwner->DoEvent(event); else CControlUI::DoEvent(event);
}

/////////////////////////////////////////////////////////////////////////////////////
//
//

CListHeaderUI::CListHeaderUI()
{
#ifdef UI_BUILD_FOR_DESIGNER
	SetUsedForPitchUpon(false);
#endif
}

LPCTSTR CListHeaderUI::GetClass() const
{
    return kListHeaderUIClassName;
}

LPVOID CListHeaderUI::GetInterface(LPCTSTR pstrName)
{
    if( _tcsicmp(pstrName, kListHeaderUIInterfaceName) == 0 ) return this;
    return CHorizontalLayoutUI::GetInterface(pstrName);
}

SIZE CListHeaderUI::EstimateSize(SIZE szAvailable)
{
    SIZE cXY = {0, m_cxyFixed.cy};
	if( cXY.cy == 0 && m_pManager != NULL ) {
		for( int it = 0; it < m_items.GetSize(); it++ ) {
			cXY.cy = MAX(cXY.cy,static_cast<CControlUI*>(m_items[it])->EstimateSize(szAvailable).cy);
		}
		int nMin = m_pManager->GetDefaultFontInfo()->tm.tmHeight + 6;
		cXY.cy = MAX(cXY.cy,nMin);
	}

    for( int it = 0; it < m_items.GetSize(); it++ ) {
        cXY.cx +=  static_cast<CControlUI*>(m_items[it])->EstimateSize(szAvailable).cx;
    }

    return cXY;
}

void CListHeaderUI::DoEvent(TEventUI& event)
{
#ifdef UI_BUILD_FOR_DESIGNER
	if (GetParent())
	{
		if (((m_pManager != NULL) && m_pManager->IsUsedAsUIDesigner() && ((GetKeyState(VK_CONTROL) < 0) || (GetKeyState(VK_SHIFT) < 0))) &&
			(( event.Type == UIEVENT_SETCURSOR )	||
			 ( event.Type == UIEVENT_BUTTONDOWN )	||
			 ( event.Type == UIEVENT_BUTTONUP )		||
			 (GetParent()->IsPitchUpon() && ( event.Type == UIEVENT_MOUSEMOVE ))))
		{
			event.pSender = GetParent();
			GetParent()->DoEvent(event);
			return;
		}
	}
#endif
	CHorizontalLayoutUI::DoEvent(event);
}

/////////////////////////////////////////////////////////////////////////////////////
//
//

CListHeaderItemUI::CListHeaderItemUI() : m_bDragable(true), m_uButtonState(0), m_iSepWidth(4),
m_uTextStyle(DT_VCENTER | DT_CENTER | DT_SINGLELINE | DT_WORD_ELLIPSIS | DT_END_ELLIPSIS), m_dwTextColor(0), m_iFont(-1), m_bShowHtml(false)
{
	SetTextPadding(CRect(2, 0, 2, 0));
    ptLastMouse.x = ptLastMouse.y = 0;
    SetMinWidth(16);
#ifdef UI_BUILD_FOR_DESIGNER
	SetUsedForPitchUpon(false);
#endif
}

LPCTSTR CListHeaderItemUI::GetClass() const
{
    return kListHeaderItemUIClassName;
}

LPVOID CListHeaderItemUI::GetInterface(LPCTSTR pstrName)
{
    if( _tcsicmp(pstrName, kListHeaderItemUIInterfaceName) == 0 ) return this;
    return CControlUI::GetInterface(pstrName);
}

UINT CListHeaderItemUI::GetControlFlags() const
{
    if( IsEnabled() && m_iSepWidth != 0 ) return UIFLAG_SETCURSOR;
    else return 0;
}

void CListHeaderItemUI::SetEnabled(bool bEnable)
{
    CControlUI::SetEnabled(bEnable);
    if( !IsEnabled() ) {
        m_uButtonState = 0;
    }
}

bool CListHeaderItemUI::IsDragable() const
{
	return m_bDragable;
}

void CListHeaderItemUI::SetDragable(bool bDragable)
{
    m_bDragable = bDragable;
    if ( !m_bDragable ) m_uButtonState &= ~UISTATE_CAPTURED;
}

DWORD CListHeaderItemUI::GetSepWidth() const
{
	return m_iSepWidth;
}

void CListHeaderItemUI::SetSepWidth(int iWidth)
{
    m_iSepWidth = iWidth;
}

DWORD CListHeaderItemUI::GetTextStyle() const
{
	return m_uTextStyle;
}

void CListHeaderItemUI::SetTextStyle(UINT uStyle)
{
    m_uTextStyle = uStyle;
    Invalidate();
}

DWORD CListHeaderItemUI::GetTextColor() const
{
	return m_dwTextColor;
}


void CListHeaderItemUI::SetTextColor(DWORD dwTextColor)
{
    m_dwTextColor = dwTextColor;
}

RECT CListHeaderItemUI::GetTextPadding() const
{
	return m_rcTextPadding;
}

void CListHeaderItemUI::SetTextPadding(RECT rc)
{
	m_rcTextPadding = rc;
	Invalidate();
}

void CListHeaderItemUI::SetFont(int index)
{
    m_iFont = index;
}

bool CListHeaderItemUI::IsShowHtml()
{
    return m_bShowHtml;
}

void CListHeaderItemUI::SetShowHtml(bool bShowHtml)
{
    if( m_bShowHtml == bShowHtml ) return;

    m_bShowHtml = bShowHtml;
    Invalidate();
}

LPCTSTR CListHeaderItemUI::GetNormalImage() const
{
	return m_sNormalImage;
}

void CListHeaderItemUI::SetNormalImage(LPCTSTR pStrImage)
{
    m_sNormalImage = pStrImage;
    Invalidate();
}

LPCTSTR CListHeaderItemUI::GetHotImage() const
{
    return m_sHotImage;
}

void CListHeaderItemUI::SetHotImage(LPCTSTR pStrImage)
{
    m_sHotImage = pStrImage;
    Invalidate();
}

LPCTSTR CListHeaderItemUI::GetPushedImage() const
{
    return m_sPushedImage;
}

void CListHeaderItemUI::SetPushedImage(LPCTSTR pStrImage)
{
    m_sPushedImage = pStrImage;
    Invalidate();
}

LPCTSTR CListHeaderItemUI::GetFocusedImage() const
{
    return m_sFocusedImage;
}

void CListHeaderItemUI::SetFocusedImage(LPCTSTR pStrImage)
{
    m_sFocusedImage = pStrImage;
    Invalidate();
}

LPCTSTR CListHeaderItemUI::GetSepImage() const
{
    return m_sSepImage;
}

void CListHeaderItemUI::SetSepImage(LPCTSTR pStrImage)
{
    m_sSepImage = pStrImage;
    Invalidate();
}

void CListHeaderItemUI::SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue)
{
    if( _tcsicmp(pstrName, _T("dragable")) == 0 ) SetDragable(_tcsicmp(pstrValue, _T("true")) == 0);
    else if( _tcsicmp(pstrName, _T("sepwidth")) == 0 ) SetSepWidth(_ttoi(pstrValue));
    else if( _tcsicmp(pstrName, _T("align")) == 0 ) {
        if( _tcsstr(pstrValue, _T("left")) != NULL ) {
            m_uTextStyle &= ~(DT_CENTER | DT_RIGHT);
            m_uTextStyle |= DT_LEFT;
        }
        if( _tcsstr(pstrValue, _T("center")) != NULL ) {
            m_uTextStyle &= ~(DT_LEFT | DT_RIGHT);
            m_uTextStyle |= DT_CENTER;
        }
        if( _tcsstr(pstrValue, _T("right")) != NULL ) {
            m_uTextStyle &= ~(DT_LEFT | DT_CENTER);
            m_uTextStyle |= DT_RIGHT;
        }
    }
    else if( _tcsicmp(pstrName, _T("endellipsis")) == 0 ) {
        if( _tcsicmp(pstrValue, _T("true")) == 0 ) m_uTextStyle |= DT_END_ELLIPSIS;
        else m_uTextStyle &= ~DT_END_ELLIPSIS;
    }    
    else if( _tcsicmp(pstrName, _T("font")) == 0 ) SetFont(_ttoi(pstrValue));
    else if( _tcsicmp(pstrName, _T("textcolor")) == 0 ) {
        if( *pstrValue == _T('#')) pstrValue = ::CharNext(pstrValue);
        LPTSTR pstr = NULL;
        DWORD clrColor = _tcstoul(pstrValue, &pstr, 16);
        SetTextColor(clrColor);
    }
	else if( _tcsicmp(pstrName, _T("textpadding")) == 0 ) {
		RECT rcTextPadding = { 0 };
		LPTSTR pstr = NULL;
		rcTextPadding.left = _tcstol(pstrValue, &pstr, 10);  ASSERT(pstr);    
		rcTextPadding.top = _tcstol(pstr + 1, &pstr, 10);    ASSERT(pstr);    
		rcTextPadding.right = _tcstol(pstr + 1, &pstr, 10);  ASSERT(pstr);    
		rcTextPadding.bottom = _tcstol(pstr + 1, &pstr, 10); ASSERT(pstr);    
		SetTextPadding(rcTextPadding);
	}
    else if( _tcsicmp(pstrName, _T("showhtml")) == 0 ) SetShowHtml(_tcsicmp(pstrValue, _T("true")) == 0);
    else if( _tcsicmp(pstrName, _T("normalimage")) == 0 ) SetNormalImage(pstrValue);
    else if( _tcsicmp(pstrName, _T("hotimage")) == 0 ) SetHotImage(pstrValue);
    else if( _tcsicmp(pstrName, _T("pushedimage")) == 0 ) SetPushedImage(pstrValue);
    else if( _tcsicmp(pstrName, _T("focusedimage")) == 0 ) SetFocusedImage(pstrValue);
    else if( _tcsicmp(pstrName, _T("sepimage")) == 0 ) SetSepImage(pstrValue);
    else CControlUI::SetAttribute(pstrName, pstrValue);
}

void CListHeaderItemUI::DoEvent(TEventUI& event)
{
    if( !IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND ) {
        if( m_pParent != NULL ) m_pParent->DoEvent(event);
        else CControlUI::DoEvent(event);
        return;
    }

    if( event.Type == UIEVENT_SETFOCUS ) 
    {
        Invalidate();
    }
    if( event.Type == UIEVENT_KILLFOCUS ) 
    {
        Invalidate();
    }
    if( event.Type == UIEVENT_BUTTONDOWN || event.Type == UIEVENT_DBLCLICK )
    {
        if( !IsEnabled() ) return;
        RECT rcSeparator = GetThumbRect();
        if( ::PtInRect(&rcSeparator, event.ptMouse) ) {
            if( m_bDragable ) {
                m_uButtonState |= UISTATE_CAPTURED;
                ptLastMouse = event.ptMouse;
            }
        }
        else {
            m_uButtonState |= UISTATE_PUSHED;
            m_pManager->SendNotify(this, _T("headerclick"));
            Invalidate();
        }
        return;
    }
    if( event.Type == UIEVENT_BUTTONUP )
    {
        if( (m_uButtonState & UISTATE_CAPTURED) != 0 ) {
            m_uButtonState &= ~UISTATE_CAPTURED;
            if( GetParent() ) 
                GetParent()->NeedParentUpdate();
        }
        else if( (m_uButtonState & UISTATE_PUSHED) != 0 ) {
            m_uButtonState &= ~UISTATE_PUSHED;
            Invalidate();
        }
        return;
    }
    if( event.Type == UIEVENT_MOUSEMOVE )
    {
        if( (m_uButtonState & UISTATE_CAPTURED) != 0 ) {
            RECT rc = m_rcItem;
            if( m_iSepWidth >= 0 ) {
                rc.right -= ptLastMouse.x - event.ptMouse.x;
            }
            else {
                rc.left -= ptLastMouse.x - event.ptMouse.x;
            }
            
            if( rc.right - rc.left > GetMinWidth() ) {
                m_cxyFixed.cx = rc.right - rc.left;
                ptLastMouse = event.ptMouse;
                if( GetParent() ) 
                    GetParent()->NeedParentUpdate();
            }
        }
        return;
    }
    if( event.Type == UIEVENT_SETCURSOR )
    {
        RECT rcSeparator = GetThumbRect();
        if( IsEnabled() && m_bDragable && ::PtInRect(&rcSeparator, event.ptMouse) ) {
            ::SetCursor(::LoadCursor(NULL, MAKEINTRESOURCE(IDC_SIZEWE)));
            return;
        }
    }
    if( event.Type == UIEVENT_MOUSEENTER )
    {
        if( IsEnabled() ) {
            m_uButtonState |= UISTATE_HOT;
            Invalidate();
        }
        return;
    }
    if( event.Type == UIEVENT_MOUSELEAVE )
    {
        if( IsEnabled() ) {
            m_uButtonState &= ~UISTATE_HOT;
            Invalidate();
        }
        return;
    }
    CControlUI::DoEvent(event);
}

SIZE CListHeaderItemUI::EstimateSize(SIZE szAvailable)
{
    if( m_cxyFixed.cy == 0 ) return CSize(m_cxyFixed.cx, m_pManager->GetDefaultFontInfo()->tm.tmHeight + 14);
    return CControlUI::EstimateSize(szAvailable);
}

RECT CListHeaderItemUI::GetThumbRect() const
{
    if( m_iSepWidth >= 0 ) return CRect(m_rcItem.right - m_iSepWidth, m_rcItem.top, m_rcItem.right, m_rcItem.bottom);
    else return CRect(m_rcItem.left, m_rcItem.top, m_rcItem.left - m_iSepWidth, m_rcItem.bottom);
}

void CListHeaderItemUI::PaintStatusImage(void* ctx)
{
	if (ctx == NULL) return;

    if( IsFocused() ) m_uButtonState |= UISTATE_FOCUSED;
    else m_uButtonState &= ~ UISTATE_FOCUSED;

    if( (m_uButtonState & UISTATE_PUSHED) != 0 ) {
        if( m_sPushedImage.IsEmpty() && !m_sNormalImage.IsEmpty() ) DrawImage(ctx, (LPCTSTR)m_sNormalImage);
        if( !DrawImage(ctx, (LPCTSTR)m_sPushedImage) ) m_sPushedImage.Empty();
    }
    else if( (m_uButtonState & UISTATE_HOT) != 0 ) {
        if( m_sHotImage.IsEmpty() && !m_sNormalImage.IsEmpty() ) DrawImage(ctx, (LPCTSTR)m_sNormalImage);
        if( !DrawImage(ctx, (LPCTSTR)m_sHotImage) ) m_sHotImage.Empty();
    }
    else if( (m_uButtonState & UISTATE_FOCUSED) != 0 ) {
        if( m_sFocusedImage.IsEmpty() && !m_sNormalImage.IsEmpty() ) DrawImage(ctx, (LPCTSTR)m_sNormalImage);
        if( !DrawImage(ctx, (LPCTSTR)m_sFocusedImage) ) m_sFocusedImage.Empty();
    }
    else {
        if( !m_sNormalImage.IsEmpty() ) {
            if( !DrawImage(ctx, (LPCTSTR)m_sNormalImage) ) m_sNormalImage.Empty();
        }
    }

    if( !m_sSepImage.IsEmpty() ) {
        RECT rcThumb = GetThumbRect();
        rcThumb.left -= m_rcItem.left;
        rcThumb.top -= m_rcItem.top;
        rcThumb.right -= m_rcItem.left;
        rcThumb.bottom -= m_rcItem.top;

        m_sSepImageModify.Empty();
        m_sSepImageModify.SmallFormat(_T("dest='%d,%d,%d,%d'"), rcThumb.left, rcThumb.top, rcThumb.right, rcThumb.bottom);
        if( !DrawImage(ctx, (LPCTSTR)m_sSepImage, (LPCTSTR)m_sSepImageModify) ) m_sSepImage.Empty();
    }
}

void CListHeaderItemUI::PaintText(void* ctx)
{
	if (ctx == NULL) return;

    if( m_dwTextColor == 0 ) m_dwTextColor = m_pManager->GetDefaultFontColor();

	RECT rcText = m_rcItem;
	rcText.left += m_rcTextPadding.left;
	rcText.top += m_rcTextPadding.top;
	rcText.right -= m_rcTextPadding.right;
	rcText.bottom -= m_rcTextPadding.bottom;

    if( m_sText.IsEmpty() ) return;
    int nLinks = 0;
    if( m_bShowHtml )
        CRenderEngine::DrawHtmlText(ctx, this, m_pManager, m_rcItem, m_sText, m_dwTextColor, \
        NULL, NULL, nLinks, DT_SINGLELINE | m_uTextStyle);
    else
        CRenderEngine::DrawText(ctx, m_pManager, m_rcItem, m_sText, m_dwTextColor, \
        m_iFont, DT_SINGLELINE | DT_WORD_ELLIPSIS | DT_END_ELLIPSIS | m_uTextStyle);
}

/////////////////////////////////////////////////////////////////////////////////////
//
//

CListElementUI::CListElementUI() : 
m_iIndex(-1),
m_pOwner(NULL), 
m_bSelected(false),
m_uButtonState(0)
{
#ifdef UI_BUILD_FOR_DESIGNER
	SetUsedForPitchUpon(false);
#endif
}

LPCTSTR CListElementUI::GetClass() const
{
    return kListElementUIClassName;
}

UINT CListElementUI::GetControlFlags() const
{
    return UIFLAG_WANTRETURN;
}

LPVOID CListElementUI::GetInterface(LPCTSTR pstrName)
{
    if( _tcsicmp(pstrName, kIListItemUIInterfaceName) == 0 ) return static_cast<IListItemUI*>(this);
	if( _tcsicmp(pstrName, kListElementUIInterfaceName) == 0 ) return static_cast<CListElementUI*>(this);
    return CControlUI::GetInterface(pstrName);
}

IListOwnerUI* CListElementUI::GetOwner()
{
    return m_pOwner;
}

void CListElementUI::SetOwner(CControlUI* pOwner)
{
    m_pOwner = static_cast<IListOwnerUI*>(pOwner->GetInterface(kIListOwnerUIInterfaceName));
}

void CListElementUI::SetVisible(bool bVisible)
{
    CControlUI::SetVisible(bVisible);
    if( !IsVisible() && m_bSelected)
    {
        m_bSelected = false;
        if( m_pOwner != NULL ) m_pOwner->SelectItem(-1);
    }
}

void CListElementUI::SetEnabled(bool bEnable)
{
    CControlUI::SetEnabled(bEnable);
    if( !IsEnabled() ) {
        m_uButtonState = 0;
    }
}

int CListElementUI::GetIndex() const
{
    return m_iIndex;
}

void CListElementUI::SetIndex(int iIndex)
{
    m_iIndex = iIndex;
}

void CListElementUI::Invalidate()
{
    if( !IsVisible() ) return;

    if( GetParent() ) {
        CContainerUI* pParentContainer = static_cast<CContainerUI*>(GetParent()->GetInterface(kContainerUIInterfaceName));
        if( pParentContainer ) {
            RECT rc = pParentContainer->GetPos();
            RECT rcInset = pParentContainer->GetInset();
            rc.left += rcInset.left;
            rc.top += rcInset.top;
            rc.right -= rcInset.right;
            rc.bottom -= rcInset.bottom;
            CScrollBarUI* pVerticalScrollBar = pParentContainer->GetVerticalScrollBar();
            if( pVerticalScrollBar && pVerticalScrollBar->IsVisible() ) rc.right -= pVerticalScrollBar->GetFixedWidth();
            CScrollBarUI* pHorizontalScrollBar = pParentContainer->GetHorizontalScrollBar();
            if( pHorizontalScrollBar && pHorizontalScrollBar->IsVisible() ) rc.bottom -= pHorizontalScrollBar->GetFixedHeight();

            RECT invalidateRc = m_rcItem;
            if( !::IntersectRect(&invalidateRc, &m_rcItem, &rc) ) 
            {
                return;
            }

            CControlUI* pParent = GetParent();
            RECT rcTemp;
            RECT rcParent;
            while( pParent = pParent->GetParent() )
            {
                rcTemp = invalidateRc;
                rcParent = pParent->GetPos();
                if( !::IntersectRect(&invalidateRc, &rcTemp, &rcParent) ) 
                {
                    return;
                }
            }

            if( m_pManager != NULL ) m_pManager->Invalidate(invalidateRc);
        }
        else {
            CControlUI::Invalidate();
        }
    }
    else {
        CControlUI::Invalidate();
    }
}

bool CListElementUI::Activate()
{
    if( !CControlUI::Activate() ) return false;
    if( m_pManager != NULL ) m_pManager->SendNotify(this, kItemActivate);
    return true;
}

bool CListElementUI::IsSelected() const
{
    return m_bSelected;
}

bool CListElementUI::Select(bool bSelect)
{
    if( !IsEnabled() ) return false;
    if( bSelect == m_bSelected ) return true;
    m_bSelected = bSelect;
    if( bSelect && m_pOwner != NULL ) m_pOwner->SelectItem(m_iIndex);
    Invalidate();

    return true;
}

bool CListElementUI::IsExpanded() const
{
    return false;
}

bool CListElementUI::Expand(bool /*bExpand = true*/)
{
    return false;
}

void CListElementUI::DoEvent(TEventUI& event)
{
    if( !IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND ) {
        if( m_pOwner != NULL ) m_pOwner->DoEvent(event);
        else CControlUI::DoEvent(event);
        return;
    }

    if( event.Type == UIEVENT_DBLCLICK )
    {
        if( IsEnabled() ) {
            Activate();
            Invalidate();
			if( m_pOwner != NULL ) m_pOwner->Activate();
        }
        return;
    }
    if( event.Type == UIEVENT_KEYDOWN && IsEnabled() )
    {
        if( event.chKey == VK_RETURN ) {
            Activate();
            Invalidate();
			if( m_pOwner != NULL ) m_pOwner->Activate();
            return;
        }
    }
    // An important twist: The list-item will send the event not to its immediate
    // parent but to the "attached" list. A list may actually embed several components
    // in its path to the item, but key-presses etc. needs to go to the actual list.
    if( m_pOwner != NULL ) m_pOwner->DoEvent(event); else CControlUI::DoEvent(event);
}

void CListElementUI::SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue)
{
    if( _tcsicmp(pstrName, _T("selected")) == 0 ) Select();
    else CControlUI::SetAttribute(pstrName, pstrValue);
}

void CListElementUI::DrawItemBk(void* ctx, const RECT& rcItem)
{
	if (ctx == NULL) return;

    ASSERT(m_pOwner);
    if( m_pOwner == NULL ) return;
    TListInfoUI* pInfo = m_pOwner->GetListInfo();
    DWORD iBackColor = 0;
    if( m_iIndex % 2 == 0 ) iBackColor = pInfo->dwBkColor;
    if( (m_uButtonState & UISTATE_HOT) != 0 ) {
        iBackColor = pInfo->dwHotBkColor;
    }
    if( IsSelected() ) {
        iBackColor = pInfo->dwSelectedBkColor;
    }
    if( !IsEnabled() ) {
        iBackColor = pInfo->dwDisabledBkColor;
    }

    if ( iBackColor != 0 ) {
        CRenderEngine::DrawColor(ctx, m_rcItem, GetAdjustColor(iBackColor));
    }

    if( !IsEnabled() ) {
        if( !pInfo->sDisabledImage.IsEmpty() ) {
            if( !DrawImage(ctx, (LPCTSTR)pInfo->sDisabledImage) ) pInfo->sDisabledImage.Empty();
            else return;
        }
    }
    if( IsSelected() ) {
        if( !pInfo->sSelectedImage.IsEmpty() ) {
            if( !DrawImage(ctx, (LPCTSTR)pInfo->sSelectedImage) ) pInfo->sSelectedImage.Empty();
            else return;
        }
    }
    if( (m_uButtonState & UISTATE_HOT) != 0 ) {
        if( !pInfo->sHotImage.IsEmpty() ) {
            if( !DrawImage(ctx, (LPCTSTR)pInfo->sHotImage) ) pInfo->sHotImage.Empty();
            else return;
        }
    }

    if( !m_sBkImage.IsEmpty() ) {
        if( m_iIndex % 2 == 0 ) {
            if( !DrawImage(ctx, (LPCTSTR)m_sBkImage) ) m_sBkImage.Empty();
        }
    }

    if( m_sBkImage.IsEmpty() ) {
        if( !pInfo->sBkImage.IsEmpty() ) {
            if( !DrawImage(ctx, (LPCTSTR)pInfo->sBkImage) ) pInfo->sBkImage.Empty();
            else return;
        }
    }

    if ( pInfo->dwLineColor != 0 ) {
        RECT rcLine = { m_rcItem.left, m_rcItem.bottom - 1, m_rcItem.right, m_rcItem.bottom - 1 };
        CRenderEngine::DrawLine(ctx, rcLine, 1, GetAdjustColor(pInfo->dwLineColor));
    }
}

/////////////////////////////////////////////////////////////////////////////////////
//
//

CListLabelElementUI::CListLabelElementUI()
{
}

LPCTSTR CListLabelElementUI::GetClass() const
{
    return kListLabelElementUIClassName;
}

LPVOID CListLabelElementUI::GetInterface(LPCTSTR pstrName)
{
    if( _tcsicmp(pstrName, kListLabelElementUIInterfaceName) == 0 ) return static_cast<CListLabelElementUI*>(this);
    return CListElementUI::GetInterface(pstrName);
}

void CListLabelElementUI::DoEvent(TEventUI& event)
{
    if( !IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND ) {
        if( m_pOwner != NULL ) m_pOwner->DoEvent(event);
        else CListElementUI::DoEvent(event);
        return;
    }

    if( event.Type == UIEVENT_BUTTONDOWN || event.Type == UIEVENT_RBUTTONDOWN )
    {
		if( IsEnabled() ) {
			if (m_bSelected)
			{
				m_bSelected = false;
				if( m_pOwner != NULL ) m_pOwner->SelectItem(-1);
			}
			Select();
			Invalidate();
			m_pManager->SendNotify(this, kItemClick);
		}
        return;
    }
    if( event.Type == UIEVENT_MOUSEMOVE ) 
    {
        return;
    }
    if( event.Type == UIEVENT_BUTTONUP )
    {
        return;
    }
    if( event.Type == UIEVENT_MOUSEENTER )
    {
        if( IsEnabled() ) {
            m_uButtonState |= UISTATE_HOT;
            Invalidate();
        }
        return;
    }
    if( event.Type == UIEVENT_MOUSELEAVE )
    {
        if( (m_uButtonState & UISTATE_HOT) != 0 ) {
            m_uButtonState &= ~UISTATE_HOT;
            Invalidate();
        }
        return;
    }
    CListElementUI::DoEvent(event);
}

SIZE CListLabelElementUI::EstimateSize(SIZE szAvailable)
{
    if( m_pOwner == NULL ) return CSize(0, 0);

    TListInfoUI* pInfo = m_pOwner->GetListInfo();
    SIZE cXY = m_cxyFixed;
    if( cXY.cy == 0 && m_pManager != NULL ) {
        cXY.cy = m_pManager->GetFontInfo(pInfo->nFont)->tm.tmHeight + 8;
        cXY.cy += pInfo->rcTextPadding.top + pInfo->rcTextPadding.bottom;
    }

    if( cXY.cx == 0 && m_pManager != NULL ) {
        RECT rcText = { 0, 0, 9999, cXY.cy };
        if( pInfo->bShowHtml ) {
            int nLinks = 0;
#if defined(UI_BUILD_FOR_WINGDI)
			CRenderEngine::DrawHtmlText(m_pManager->GetPaintDC(), this, m_pManager, rcText, m_sText, 0, NULL, NULL, nLinks, DT_SINGLELINE | DT_CALCRECT | pInfo->uTextStyle & ~DT_RIGHT & ~DT_CENTER);
#elif defined(UI_BUILD_FOR_SKIA)
			CRenderEngine::DrawHtmlText(m_pManager->GetSkiaContext(), this, m_pManager, rcText, m_sText, 0, NULL, NULL, nLinks, DT_SINGLELINE | DT_CALCRECT | pInfo->uTextStyle & ~DT_RIGHT & ~DT_CENTER);
#endif
		}
        else {
#if defined(UI_BUILD_FOR_WINGDI)
            CRenderEngine::DrawText(m_pManager->GetPaintDC(), m_pManager, rcText, m_sText, 0, pInfo->nFont, DT_SINGLELINE | DT_CALCRECT | pInfo->uTextStyle & ~DT_RIGHT & ~DT_CENTER);
#elif defined(UI_BUILD_FOR_SKIA)
			CRenderEngine::DrawText(m_pManager->GetSkiaContext(), m_pManager, rcText, m_sText, 0, pInfo->nFont, DT_SINGLELINE | DT_CALCRECT | pInfo->uTextStyle & ~DT_RIGHT & ~DT_CENTER);
#endif
        }
        cXY.cx = rcText.right - rcText.left + pInfo->rcTextPadding.left + pInfo->rcTextPadding.right;        
    }

    return cXY;
}

void CListLabelElementUI::DoPaint(void* ctx, const RECT& rcPaint)
{
	if (ctx == NULL) return;

    if( !::IntersectRect(&m_rcPaint, &rcPaint, &m_rcItem) ) return;
    DrawItemBk(ctx, m_rcItem);
    DrawItemText(ctx, m_rcItem);
}

void CListLabelElementUI::DrawItemText(void* ctx, const RECT& rcItem)
{
	if (ctx == NULL) return;
    if( m_sText.IsEmpty() ) return;
    if( m_pOwner == NULL ) return;

    TListInfoUI* pInfo = m_pOwner->GetListInfo();
    DWORD iTextColor = pInfo->dwTextColor;
    if( (m_uButtonState & UISTATE_HOT) != 0 ) {
        iTextColor = pInfo->dwHotTextColor;
    }
    if( IsSelected() ) {
        iTextColor = pInfo->dwSelectedTextColor;
    }
    if( !IsEnabled() ) {
        iTextColor = pInfo->dwDisabledTextColor;
    }
    int nLinks = 0;
    RECT rcText = rcItem;
    rcText.left += pInfo->rcTextPadding.left;
    rcText.right -= pInfo->rcTextPadding.right;
    rcText.top += pInfo->rcTextPadding.top;
    rcText.bottom -= pInfo->rcTextPadding.bottom;

    if( pInfo->bShowHtml )
	{
        CRenderEngine::DrawHtmlText(ctx, this, m_pManager, rcText, m_sText, iTextColor, \
        NULL, NULL, nLinks, DT_SINGLELINE | DT_WORD_ELLIPSIS | DT_END_ELLIPSIS | pInfo->uTextStyle);
	}
    else
        CRenderEngine::DrawText(ctx, m_pManager, rcText, m_sText, iTextColor, \
        pInfo->nFont, DT_SINGLELINE | DT_WORD_ELLIPSIS | DT_END_ELLIPSIS | pInfo->uTextStyle);
}


/////////////////////////////////////////////////////////////////////////////////////
//
//

CListTextElementUI::CListTextElementUI() : m_nLinks(0), m_nHoverLink(-1), m_pOwner(NULL)
{
    ::ZeroMemory(&m_rcLinks, sizeof(m_rcLinks));
}

CListTextElementUI::~CListTextElementUI()
{
    CStdString* pText;
    for( int it = 0; it < m_aTexts.GetSize(); it++ ) {
        pText = static_cast<CStdString*>(m_aTexts[it]);
        if( pText ) delete pText;
    }
    m_aTexts.Empty();
}

LPCTSTR CListTextElementUI::GetClass() const
{
    return kListTextElementUIClassName;
}

LPVOID CListTextElementUI::GetInterface(LPCTSTR pstrName)
{
    if( _tcsicmp(pstrName, kListTextElementUIInterfaceName) == 0 ) return static_cast<CListTextElementUI*>(this);
    return CListLabelElementUI::GetInterface(pstrName);
}

UINT CListTextElementUI::GetControlFlags() const
{
    return UIFLAG_WANTRETURN | ( (IsEnabled() && m_nLinks > 0) ? UIFLAG_SETCURSOR : 0);
}

LPCTSTR CListTextElementUI::GetText(int iIndex) const
{
    CStdString* pText = static_cast<CStdString*>(m_aTexts.GetAt(iIndex));
    if( pText ) return pText->GetData();
    return NULL;
}

void CListTextElementUI::SetText(int iIndex, LPCTSTR pstrText)
{
    if( m_pOwner == NULL ) return;
    TListInfoUI* pInfo = m_pOwner->GetListInfo();
    if( iIndex < 0 || iIndex >= pInfo->nColumns ) return;
    while( m_aTexts.GetSize() < pInfo->nColumns ) { m_aTexts.Add(NULL); }

    CStdString* pText = static_cast<CStdString*>(m_aTexts[iIndex]);
    if( (pText == NULL && pstrText == NULL) || (pText && *pText == pstrText) ) return;

    if( pText ) delete pText;
    m_aTexts.SetAt(iIndex, new CStdString(pstrText));
    Invalidate();
}

void CListTextElementUI::SetOwner(CControlUI* pOwner)
{
    CListElementUI::SetOwner(pOwner);
    m_pOwner = static_cast<IListUI*>(pOwner->GetInterface(kIListUIInterfaceName));
}

CStdString* CListTextElementUI::GetLinkContent(int iIndex)
{
    if( iIndex >= 0 && iIndex < m_nLinks ) return &m_sLinks[iIndex];
    return NULL;
}

void CListTextElementUI::DoEvent(TEventUI& event)
{
    if( !IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND ) {
        if( m_pOwner != NULL ) m_pOwner->DoEvent(event);
        else CListLabelElementUI::DoEvent(event);
        return;
    }

    // When you hover over a link
    if( event.Type == UIEVENT_SETCURSOR ) {
        for( int i = 0; i < m_nLinks; i++ ) {
            if( ::PtInRect(&m_rcLinks[i], event.ptMouse) ) {
                ::SetCursor(::LoadCursor(NULL, MAKEINTRESOURCE(IDC_HAND)));
                return;
            }
        }      
    }
    if( event.Type == UIEVENT_BUTTONUP && IsEnabled() ) {
        for( int i = 0; i < m_nLinks; i++ ) {
            if( ::PtInRect(&m_rcLinks[i], event.ptMouse) ) {
                m_pManager->SendNotify(this, _T("link"), i);
                return;
            }
        }
    }
    if( m_nLinks > 0 && event.Type == UIEVENT_MOUSEMOVE ) {
        int nHoverLink = -1;
        for( int i = 0; i < m_nLinks; i++ ) {
            if( ::PtInRect(&m_rcLinks[i], event.ptMouse) ) {
                nHoverLink = i;
                break;
            }
        }

        if(m_nHoverLink != nHoverLink) {
            Invalidate();
            m_nHoverLink = nHoverLink;
        }
    }
    if( m_nLinks > 0 && event.Type == UIEVENT_MOUSELEAVE ) {
        if(m_nHoverLink != -1) {
            Invalidate();
            m_nHoverLink = -1;
        }
    }
    CListLabelElementUI::DoEvent(event);
}

SIZE CListTextElementUI::EstimateSize(SIZE szAvailable)
{
    TListInfoUI* pInfo = NULL;
    if( m_pOwner ) pInfo = m_pOwner->GetListInfo();

    SIZE cXY = m_cxyFixed;
    if( cXY.cy == 0 && m_pManager != NULL ) {
        cXY.cy = m_pManager->GetFontInfo(pInfo->nFont)->tm.tmHeight + 8;
        if( pInfo ) cXY.cy += pInfo->rcTextPadding.top + pInfo->rcTextPadding.bottom;
    }

    return cXY;
}

void CListTextElementUI::DrawItemText(void* ctx, const RECT& rcItem)
{
	if (ctx == NULL) return;
    if( m_pOwner == NULL ) return;

    TListInfoUI* pInfo = m_pOwner->GetListInfo();
    DWORD iTextColor = pInfo->dwTextColor;

    if( (m_uButtonState & UISTATE_HOT) != 0 ) {
        iTextColor = pInfo->dwHotTextColor;
    }
    if( IsSelected() ) {
        iTextColor = pInfo->dwSelectedTextColor;
    }
    if( !IsEnabled() ) {
        iTextColor = pInfo->dwDisabledTextColor;
    }
    IListCallbackUI* pCallback = m_pOwner->GetTextCallback();
    //ASSERT(pCallback);
    //if( pCallback == NULL ) return;

    m_nLinks = 0;
    int nLinks = lengthof(m_rcLinks);
    for( int i = 0; i < pInfo->nColumns; i++ )
    {
        RECT rcItem = { pInfo->rcColumn[i].left, m_rcItem.top, pInfo->rcColumn[i].right, m_rcItem.bottom };
        rcItem.left += pInfo->rcTextPadding.left;
        rcItem.right -= pInfo->rcTextPadding.right;
        rcItem.top += pInfo->rcTextPadding.top;
        rcItem.bottom -= pInfo->rcTextPadding.bottom;
        LPCTSTR pstrText = NULL;
        if( pCallback ) pstrText = pCallback->GetItemText(this, m_iIndex, i);
        else pstrText = GetText(i);

        if( pInfo->bShowHtml )
            CRenderEngine::DrawHtmlText(ctx, this, m_pManager, rcItem, pstrText, iTextColor, \
            &m_rcLinks[m_nLinks], &m_sLinks[m_nLinks], nLinks, DT_SINGLELINE | DT_WORD_ELLIPSIS | DT_END_ELLIPSIS | pInfo->uTextStyle);
        else
            CRenderEngine::DrawText(ctx, m_pManager, rcItem, pstrText, iTextColor, \
            pInfo->nFont, DT_SINGLELINE | DT_WORD_ELLIPSIS | DT_END_ELLIPSIS | pInfo->uTextStyle);

        m_nLinks += nLinks;
        nLinks = lengthof(m_rcLinks) - m_nLinks; 
    }
    for( int i = m_nLinks; i < lengthof(m_rcLinks); i++ ) {
        ::ZeroMemory(m_rcLinks + i, sizeof(RECT));
        ((CStdString*)(m_sLinks + i))->Empty();
    }
}


/////////////////////////////////////////////////////////////////////////////////////
//
//
//
//CListExpandElementUI::CListExpandElementUI() : m_bExpanded(false), m_bHideSelf(false), m_pSubControl(NULL)
//{
//    ::ZeroMemory(&m_rcExpander, sizeof(m_rcExpander));
//}
//
//CListExpandElementUI::~CListExpandElementUI()
//{
//    if( m_pSubControl ) delete m_pSubControl;
//}
//
//LPCTSTR CListExpandElementUI::GetClass() const
//{
//    return kListExpandElementUIClassName;
//}
//
//LPVOID CListExpandElementUI::GetInterface(LPCTSTR pstrName)
//{
//    if( _tcsicmp(pstrName, kListExpandElementUIInterfaceName) == 0 ) return static_cast<CListExpandElementUI*>(this);
//    return CListLabelElementUI::GetInterface(pstrName);
//}
//
//bool CListExpandElementUI::IsExpanded() const
//{
//    return m_bExpanded;
//}
//
//bool CListExpandElementUI::Expand(bool bExpand)
//{
//    ASSERT(m_pOwner);
//    if( m_pOwner == NULL ) return false;  
//    if( bExpand == m_bExpanded ) return true;
//    m_bExpanded = bExpand;
//    if( !m_pOwner->ExpandItem(m_iIndex, bExpand) ) return false;
//    // We need to manage the "expanding items", which are actually embedded controls
//    // that we selectively display or not.
//    if( bExpand ) 
//    {
//        if( m_pSubControl ) delete m_pSubControl;
//        m_pSubControl = NULL;
//
//        if( m_pManager != NULL ) m_pManager->SendNotify(this, _T("itemexpand"));
//        if( m_pSubControl ) m_pManager->InitControls(m_pSubControl, this);
//    }
//    else
//    {
//        if( m_pManager != NULL ) m_pManager->SendNotify(this, _T("itemcollapse"));
//    }
//    return true;
//}
//
//bool CListExpandElementUI::GetExpandHideSelf() const
//{
//    return m_bHideSelf;
//}
//
//void CListExpandElementUI::SetExpandHideSelf(bool bHideSelf)
//{
//    m_bHideSelf = bHideSelf;
//    NeedParentUpdate();
//}
//
//RECT CListExpandElementUI::GetExpanderRect() const
//{
//    return m_rcExpander;
//}
//
//void CListExpandElementUI::SetExpanderRect(RECT rc)
//{
//    m_rcExpander = rc;
//}
//
//void CListExpandElementUI::SetExpandItem(CControlUI* pControl)
//{
//    if (m_pSubControl ) delete m_pSubControl;
//    m_pSubControl = pControl;
//}
//
//void CListExpandElementUI::SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue)
//{
//    if( _tcsicmp(pstrName, _T("expander")) == 0 ) {
//        RECT rcExpander = { 0 };
//        LPTSTR pstr = NULL;
//        rcExpander.left = _tcstol(pstrValue, &pstr, 10);  ASSERT(pstr);    
//        rcExpander.top = _tcstol(pstr + 1, &pstr, 10);    ASSERT(pstr);    
//        rcExpander.right = _tcstol(pstr + 1, &pstr, 10);  ASSERT(pstr);    
//        rcExpander.bottom = _tcstol(pstr + 1, &pstr, 10); ASSERT(pstr);    
//        SetExpanderRect(rcExpander);
//    }
//    else if( _tcsicmp(pstrName, _T("hideself")) == 0 ) SetExpandHideSelf(_tcsicmp(pstrValue, _T("true")) == 0);
//    else CListTextElementUI::SetAttribute(pstrName, pstrValue);
//}
//
//void CListExpandElementUI::DoEvent(TEventUI& event)
//{
//    if( !IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND ) {
//        if( m_pOwner != NULL ) m_pOwner->DoEvent(event);
//        else CListTextElementUI::DoEvent(event);
//        return;
//    }
//    if( event.Type == UIEVENT_BUTTONDOWN || event.Type == UIEVENT_RBUTTONDOWN )
//    {
//        if( IsEnabled() ) {
//            Select();
//            Invalidate();
//			m_pManager->SendNotify(this, kItemClick);
//        }
//        return;
//    }
//    if( event.Type == UIEVENT_MOUSEMOVE ) 
//    {
//        return;
//    }
//    if( event.Type == UIEVENT_BUTTONUP )
//    {
//        if( m_pOwner == NULL ) return;
//        TListInfoUI* pInfo = m_pOwner->GetListInfo();
//        RECT rcExpander = { m_rcItem.left + m_rcExpander.left, m_rcItem.top + m_rcExpander.top, 
//            m_rcItem.left + m_rcExpander.right, m_rcItem.top + m_rcExpander.bottom };
//        if( ::PtInRect(&rcExpander, event.ptMouse) ) Expand(!m_bExpanded);
//        return;
//    }
//    if( event.Type == UIEVENT_KEYDOWN )
//    {
//        switch( event.chKey ) {
//      case VK_LEFT:
//          Expand(false);
//          return;
//      case VK_RIGHT:
//          Expand(true);
//          return;
//        }
//    }
//    if( event.Type == UIEVENT_MOUSEENTER )
//    {
//        if( (m_uButtonState & UISTATE_HOT) == 0  ) {
//            m_uButtonState |= UISTATE_HOT;
//            Invalidate();
//        }
//        return;
//    }
//    if( event.Type == UIEVENT_MOUSELEAVE )
//    {
//        if( (m_uButtonState & UISTATE_HOT) != 0 ) {
//            m_uButtonState &= ~UISTATE_HOT;
//            Invalidate();
//        }
//        return;
//    }
//    CListTextElementUI::DoEvent(event);
//}
//
//SIZE CListExpandElementUI::EstimateSize(SIZE szAvailable)
//{
//    if( m_bExpanded && m_pSubControl != NULL ) {
//        if( m_bHideSelf ) return m_pSubControl->EstimateSize(szAvailable);
//        
//        SIZE sz = m_pSubControl->EstimateSize(szAvailable);
//        if( sz.cy < m_pSubControl->GetMinHeight() ) sz.cy = m_pSubControl->GetMinHeight();
//        if( sz.cy > m_pSubControl->GetMaxHeight() ) sz.cy = m_pSubControl->GetMaxHeight();
//
//        if( m_cxyFixed.cy == 0 ) sz.cy += CListTextElementUI::EstimateSize(szAvailable).cy;
//        else sz.cy += m_cxyFixed.cy;
//        return CSize(m_cxyFixed.cx, sz.cy);
//    }
//    else
//    {
//        return CListTextElementUI::EstimateSize(szAvailable);
//    }   
//}
//
//void CListExpandElementUI::SetVisible(bool bVisible)
//{
//    if( m_bVisible == bVisible ) return;
//    CControlUI::SetVisible(bVisible);
//
//    if( m_bExpanded && m_pSubControl != NULL ) {
//        m_pSubControl->SetInternVisible(bVisible);
//    }
//}
//
//void CListExpandElementUI::SetInternVisible(bool bVisible)
//{
//    if( m_bExpanded && m_pSubControl != NULL ) {
//        m_pSubControl->SetInternVisible(bVisible);
//    }
//    CControlUI::SetInternVisible(bVisible);
//}
//
//void CListExpandElementUI::SetMouseEnabled(bool bEnabled)
//{
//    m_bMouseEnabled = bEnabled;
//
//    if( m_bExpanded && m_pSubControl != NULL ) {
//        m_pSubControl->SetMouseEnabled(bEnabled);
//    }
//}
//
//void CListExpandElementUI::SetPos(RECT rc)
//{
//    CListTextElementUI::SetPos(rc);
//    if( m_pSubControl != NULL ) {
//        if( m_bHideSelf ) {
//            RECT rcPadding = m_pSubControl->GetPadding();
//            RECT rcSubItem = { rc.left + rcPadding.left, rc.top + rcPadding.top, 
//                rc.right - rcPadding.right, rc.bottom - rcPadding.bottom };
//            m_pSubControl->SetPos(rcSubItem);
//        }
//        else {
//            SIZE sz = m_pSubControl->EstimateSize(CSize(rc.right - rc.left, rc.bottom - rc.top));
//            if( sz.cy < m_pSubControl->GetMinHeight() ) sz.cy = m_pSubControl->GetMinHeight();
//            if( sz.cy > m_pSubControl->GetMaxHeight() ) sz.cy = m_pSubControl->GetMaxHeight();
//            RECT rcPadding = m_pSubControl->GetPadding();
//
//            if( sz.cy == 0 && m_cxyFixed.cy == 0 ) {
//                RECT rcSubItem = { rc.left + rcPadding.left, (rc.bottom + rc.top) / 2 + rcPadding.top, 
//                    rc.right - rcPadding.right, rc.bottom - rcPadding.bottom };
//                m_pSubControl->SetPos(rcSubItem);
//            }
//            else if( sz.cy != 0 && m_cxyFixed.cy == 0 ) {
//                RECT rcSubItem = { rc.left + rcPadding.left, rc.bottom - sz.cy - rcPadding.bottom, 
//                    rc.right - rcPadding.right, rc.bottom - rcPadding.bottom };
//                m_pSubControl->SetPos(rcSubItem);
//            }
//            else {
//                RECT rcSubItem = { rc.left + rcPadding.left, rc.top + m_cxyFixed.cy + rcPadding.top, 
//                    rc.right - rcPadding.right, rc.bottom - rcPadding.bottom };
//                m_pSubControl->SetPos(rcSubItem);
//            }
//        }
//    }
//}
//
//void CListExpandElementUI::DoPaint(void* ctx, const RECT& rcPaint)
//{
//	if (ctx == NULL) return;
//
//    CListTextElementUI::DoPaint(ctx, rcPaint);
//
//    if( m_bExpanded && m_pSubControl != NULL ) {
//        m_pSubControl->DoPaint(ctx, rcPaint);
//    }
//}
//
//void CListExpandElementUI::SetManager(CPaintManagerUI* pManager, CControlUI* pParent, bool bInit)
//{
//    if( m_bExpanded && m_pSubControl != NULL ) m_pSubControl->SetManager(pManager, this, bInit);
//    CListTextElementUI::SetManager(pManager, pParent, bInit);
//}
//
//CControlUI* CListExpandElementUI::FindControl(FINDCONTROLPROC Proc, LPVOID pData, UINT uFlags)
//{
//    CControlUI* pResult = NULL;
//    if( m_bExpanded && m_pSubControl != NULL ) pResult = m_pSubControl->FindControl(Proc, pData, uFlags);
//    if( pResult == NULL ) pResult = CListTextElementUI::FindControl(Proc, pData, uFlags);
//    return pResult;
//}
//
//void CListExpandElementUI::DrawItemText(void* ctx, const RECT& rcItem)
//{
//	if (ctx == NULL) return;
//
//    DrawItemBk(ctx, rcItem);
//    if( m_bExpanded && m_pSubControl != NULL && m_bHideSelf ) return;
//
//    if( m_pOwner == NULL ) return;
//    TListInfoUI* pInfo = m_pOwner->GetListInfo();
//    DWORD iTextColor = pInfo->dwTextColor;
//    if( (m_uButtonState & UISTATE_HOT) != 0 ) {
//        iTextColor = pInfo->dwHotTextColor;
//    }
//    if( IsSelected() ) {
//        iTextColor = pInfo->dwSelectedTextColor;
//    }
//    if( !IsEnabled() ) {
//        iTextColor = pInfo->dwDisabledTextColor;
//    }
//
//    IListCallbackUI* pCallback = m_pOwner->GetTextCallback();
//    ASSERT(pCallback);
//    if( pCallback == NULL ) return;
//    m_nLinks = 0;
//    CStdString sColText;
//    int nLinks = lengthof(m_rcLinks);
//    for( int i = 0; i < pInfo->nColumns; i++ )
//    {
//        // Paint text
//        RECT rcItem = { pInfo->rcColumn[i].left, m_rcItem.top, pInfo->rcColumn[i].right, m_rcItem.bottom };
//        if( m_bExpanded && m_pSubControl != NULL ) rcItem.bottom -= m_pSubControl->GetHeight();
//        rcItem.left += pInfo->rcTextPadding.left;
//        rcItem.right -= pInfo->rcTextPadding.right;
//        rcItem.top += pInfo->rcTextPadding.top;
//        rcItem.bottom -= pInfo->rcTextPadding.bottom;
//
//        LPCTSTR pstrText = pCallback->GetItemText(this, m_iIndex, i);
//        if( pInfo->bShowHtml )
//            CRenderEngine::DrawHtmlText(ctx, this, m_pManager, rcItem, pstrText, iTextColor, \
//            m_rcLinks, m_sLinks, nLinks, DT_SINGLELINE | DT_WORD_ELLIPSIS | DT_END_ELLIPSIS | pInfo->uTextStyle);
//        else
//            CRenderEngine::DrawText(ctx, m_pManager, rcItem, pstrText, iTextColor, \
//            pInfo->nFont, DT_SINGLELINE | DT_WORD_ELLIPSIS | DT_END_ELLIPSIS | pInfo->uTextStyle);
//
//        if( nLinks > 0 ) m_nLinks = nLinks, nLinks = 0; 
//        else nLinks = lengthof(m_rcLinks);
//    }
//}


/////////////////////////////////////////////////////////////////////////////////////
//
//

CListContainerElementUI::CListContainerElementUI() : 
m_iIndex(-1),
m_pOwner(NULL), 
m_bSelected(false),
m_uButtonState(0)
{
}

LPCTSTR CListContainerElementUI::GetClass() const
{
    return kListContainerElementUIClassName;
}

UINT CListContainerElementUI::GetControlFlags() const
{
    return UIFLAG_WANTRETURN;
}

LPVOID CListContainerElementUI::GetInterface(LPCTSTR pstrName)
{
    if( _tcsicmp(pstrName, kIListItemUIInterfaceName) == 0 ) return static_cast<IListItemUI*>(this);
	if( _tcsicmp(pstrName, kListContainerElementUIInterfaceName) == 0 ) return static_cast<CListContainerElementUI*>(this);
    return CContainerUI::GetInterface(pstrName);
}

IListOwnerUI* CListContainerElementUI::GetOwner()
{
    return m_pOwner;
}

void CListContainerElementUI::SetOwner(CControlUI* pOwner)
{
    m_pOwner = static_cast<IListOwnerUI*>(pOwner->GetInterface(kIListOwnerUIInterfaceName));
}

void CListContainerElementUI::SetVisible(bool bVisible)
{
    CContainerUI::SetVisible(bVisible);
    if( !IsVisible() && m_bSelected)
    {
        m_bSelected = false;
        if( m_pOwner != NULL ) m_pOwner->SelectItem(-1);
    }
}

void CListContainerElementUI::SetEnabled(bool bEnable)
{
    CControlUI::SetEnabled(bEnable);
    if( !IsEnabled() ) {
        m_uButtonState = 0;
    }
}

int CListContainerElementUI::GetIndex() const
{
    return m_iIndex;
}

void CListContainerElementUI::SetIndex(int iIndex)
{
    m_iIndex = iIndex;
}

void CListContainerElementUI::Invalidate()
{
    if( !IsVisible() ) return;

    if( GetParent() ) {
        CContainerUI* pParentContainer = static_cast<CContainerUI*>(GetParent()->GetInterface(kContainerUIInterfaceName));
        if( pParentContainer ) {
            RECT rc = pParentContainer->GetPos();
            RECT rcInset = pParentContainer->GetInset();
            rc.left += rcInset.left;
            rc.top += rcInset.top;
            rc.right -= rcInset.right;
            rc.bottom -= rcInset.bottom;
            CScrollBarUI* pVerticalScrollBar = pParentContainer->GetVerticalScrollBar();
            if( pVerticalScrollBar && pVerticalScrollBar->IsVisible() ) rc.right -= pVerticalScrollBar->GetFixedWidth();
            CScrollBarUI* pHorizontalScrollBar = pParentContainer->GetHorizontalScrollBar();
            if( pHorizontalScrollBar && pHorizontalScrollBar->IsVisible() ) rc.bottom -= pHorizontalScrollBar->GetFixedHeight();

            RECT invalidateRc = m_rcItem;
            if( !::IntersectRect(&invalidateRc, &m_rcItem, &rc) ) 
            {
                return;
            }

            CControlUI* pParent = GetParent();
            RECT rcTemp;
            RECT rcParent;
            while( pParent = pParent->GetParent() )
            {
                rcTemp = invalidateRc;
                rcParent = pParent->GetPos();
                if( !::IntersectRect(&invalidateRc, &rcTemp, &rcParent) ) 
                {
                    return;
                }
            }

            if( m_pManager != NULL ) m_pManager->Invalidate(invalidateRc);
        }
        else {
            CContainerUI::Invalidate();
        }
    }
    else {
        CContainerUI::Invalidate();
    }
}

bool CListContainerElementUI::Activate()
{
    if( !CContainerUI::Activate() ) return false;
    if( m_pManager != NULL ) m_pManager->SendNotify(this, kItemActivate);
    return true;
}

bool CListContainerElementUI::IsSelected() const
{
    return m_bSelected;
}

bool CListContainerElementUI::Select(bool bSelect)
{
    if( !IsEnabled() ) return false;
    if( bSelect == m_bSelected ) return true;
    m_bSelected = bSelect;
    if( bSelect && m_pOwner != NULL ) m_pOwner->SelectItem(m_iIndex);
    Invalidate();

    return true;
}

bool CListContainerElementUI::IsExpanded() const
{
    return false;
}

bool CListContainerElementUI::Expand(bool /*bExpand = true*/)
{
    return false;
}

void CListContainerElementUI::DoEvent(TEventUI& event)
{
    if( !IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND ) {
        if( m_pOwner != NULL ) m_pOwner->DoEvent(event);
        else CContainerUI::DoEvent(event);
        return;
    }

    if( event.Type == UIEVENT_DBLCLICK )
    {
        if( IsEnabled() ) {
            Activate();
            Invalidate();
        }
        return;
    }
    if( event.Type == UIEVENT_KEYDOWN && IsEnabled() )
    {
        if( event.chKey == VK_RETURN ) {
            Activate();
            Invalidate();
            return;
        }
    }
    if( event.Type == UIEVENT_BUTTONDOWN || event.Type == UIEVENT_RBUTTONDOWN )
    {
		if( IsEnabled() ){
			if (m_bSelected)
			{
				m_bSelected = false;
				if( m_pOwner != NULL ) m_pOwner->SelectItem(-1);
			}

			Select();
			Invalidate();
			m_pManager->SendNotify(this, kItemClick);
		}
		return;
    }
    if( event.Type == UIEVENT_BUTTONUP ) 
    {
        return;
    }
    if( event.Type == UIEVENT_MOUSEMOVE )
    {
        return;
    }
    if( event.Type == UIEVENT_MOUSEENTER )
    {
        if( IsEnabled() ) {
            m_uButtonState |= UISTATE_HOT;
            Invalidate();
        }
        return;
    }
    if( event.Type == UIEVENT_MOUSELEAVE )
    {
        if( (m_uButtonState & UISTATE_HOT) != 0 ) {
            m_uButtonState &= ~UISTATE_HOT;
            Invalidate();
        }
        return;
    }

    // An important twist: The list-item will send the event not to its immediate
    // parent but to the "attached" list. A list may actually embed several components
    // in its path to the item, but key-presses etc. needs to go to the actual list.
    if( m_pOwner != NULL ) m_pOwner->DoEvent(event); else CControlUI::DoEvent(event);
}

void CListContainerElementUI::SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue)
{
    if( _tcsicmp(pstrName, _T("selected")) == 0 ) Select();
    else CContainerUI::SetAttribute(pstrName, pstrValue);
}

void CListContainerElementUI::DoPaint(void* ctx, const RECT& rcPaint)
{
	if (ctx == NULL) return;

    if( !::IntersectRect(&m_rcPaint, &rcPaint, &m_rcItem) ) return;
    DrawItemBk(ctx, m_rcItem);
    CContainerUI::DoPaint(ctx, rcPaint);
}

void CListContainerElementUI::DrawItemText(void* ctx, const RECT& rcItem)
{
    return;
}

void CListContainerElementUI::DrawItemBk(void* ctx, const RECT& rcItem)
{
	if (ctx == NULL) return;

    ASSERT(m_pOwner);
    if( m_pOwner == NULL ) return;
    TListInfoUI* pInfo = m_pOwner->GetListInfo();
    DWORD iBackColor = 0;
    if( m_iIndex % 2 == 0 ) iBackColor = pInfo->dwBkColor;

    if( (m_uButtonState & UISTATE_HOT) != 0 ) {
        iBackColor = pInfo->dwHotBkColor;
    }
    if( IsSelected() ) {
        iBackColor = pInfo->dwSelectedBkColor;
    }
    if( !IsEnabled() ) {
        iBackColor = pInfo->dwDisabledBkColor;
    }
    if ( iBackColor != 0 ) {
        CRenderEngine::DrawColor(ctx, m_rcItem, GetAdjustColor(iBackColor));
    }

    if( !IsEnabled() ) {
        if( !pInfo->sDisabledImage.IsEmpty() ) {
            if( !DrawImage(ctx, (LPCTSTR)pInfo->sDisabledImage) ) pInfo->sDisabledImage.Empty();
            else return;
        }
    }
    if( IsSelected() ) {
        if( !pInfo->sSelectedImage.IsEmpty() ) {
            if( !DrawImage(ctx, (LPCTSTR)pInfo->sSelectedImage) ) pInfo->sSelectedImage.Empty();
            else return;
        }
    }
    if( (m_uButtonState & UISTATE_HOT) != 0 ) {
        if( !pInfo->sHotImage.IsEmpty() ) {
            if( !DrawImage(ctx, (LPCTSTR)pInfo->sHotImage) ) pInfo->sHotImage.Empty();
            else return;
        }
    }
    if( !m_sBkImage.IsEmpty() ) {
        if( m_iIndex % 2 == 0 ) {
            if( !DrawImage(ctx, (LPCTSTR)m_sBkImage) ) m_sBkImage.Empty();
        }
    }

    if( m_sBkImage.IsEmpty() ) {
        if( !pInfo->sBkImage.IsEmpty() ) {
            if( !DrawImage(ctx, (LPCTSTR)pInfo->sBkImage) ) pInfo->sBkImage.Empty();
            else return;
        }
    }

    if ( pInfo->dwLineColor != 0 ) {
        RECT rcLine = { m_rcItem.left, m_rcItem.bottom - 1, m_rcItem.right, m_rcItem.bottom - 1 };
        CRenderEngine::DrawLine(ctx, rcLine, 1, GetAdjustColor(pInfo->dwLineColor));
    }
}

} // namespace DuiLib
