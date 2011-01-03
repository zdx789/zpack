
// zpEditorView.cpp : implementation of the CzpEditorView class
//

#include "stdafx.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "zpEditor.h"
#endif

#include "zpEditorDoc.h"
#include "zpEditorView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CzpEditorView

IMPLEMENT_DYNCREATE(CzpEditorView, CListView)

BEGIN_MESSAGE_MAP(CzpEditorView, CListView)
	ON_WM_STYLECHANGED()
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONUP()
END_MESSAGE_MAP()

// CzpEditorView construction/destruction

CzpEditorView::CzpEditorView()
{
	// TODO: add construction code here

}

CzpEditorView::~CzpEditorView()
{
}

BOOL CzpEditorView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CListView::PreCreateWindow(cs);
}

void CzpEditorView::OnInitialUpdate()
{
	CListView::OnInitialUpdate();

	CListCtrl& listCtrl = GetListCtrl();

	HMODULE hShell32 = LoadLibrary("shell32.dll");
	if (hShell32 != NULL)
	{
		typedef BOOL (WINAPI * FII_PROC)(BOOL fFullInit);
		FII_PROC FileIconInit = (FII_PROC)GetProcAddress(hShell32, (LPCSTR)660);
		if (FileIconInit != NULL)
		{
			FileIconInit(TRUE);
		}
	}
	HIMAGELIST imageList;
	::Shell_GetImageLists(NULL, &imageList);
	listCtrl.SetImageList(CImageList::FromHandle(imageList), LVSIL_SMALL);

	listCtrl.ModifyStyle(0, LVS_REPORT );
	listCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);

	LVCOLUMN lc;
	lc.mask = LVCF_WIDTH | LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM;
	lc.cx = 200;
	lc.fmt = LVCFMT_LEFT;
	lc.pszText = "Filename";
	lc.cchTextMax = 256;
	lc.iSubItem = 0;
	listCtrl.InsertColumn(0, &lc);

	lc.cx = 100;
	lc.pszText = "Size";
	lc.iSubItem = 1;
	listCtrl.InsertColumn(1, &lc);
}

void CzpEditorView::OnRButtonUp(UINT /* nFlags */, CPoint point)
{
	ClientToScreen(&point);
	OnContextMenu(this, point);
}

void CzpEditorView::OnContextMenu(CWnd* /* pWnd */, CPoint point)
{
#ifndef SHARED_HANDLERS
	theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
#endif
}

void CzpEditorView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	CListCtrl& listCtrl = GetListCtrl();
	listCtrl.DeleteAllItems();

	const ZpExplorer& explorer = GetDocument()->GetZpExplorer();
	const ZpNode* currentNode = explorer.currentNode();
	DWORD index = 0;
	for (std::list<ZpNode>::const_iterator iter = currentNode->children.cbegin();
		iter != currentNode->children.cend();
		++iter)
	{
		const std::string& nodeName = iter->name;
		int iconIndex = 3;	//2 suppose to be fold icon
		if (!iter->isDirectory)
		{
			std::string fileExt;
			size_t pos = nodeName.find_last_of('.');
			if (pos == std::string::npos)
			{
				fileExt = "holycrap";	//can not be empty, or will get some icon like your c:
			}
			else
			{
				fileExt = nodeName.substr(pos, iter->name.length() - pos);
			}
			SHFILEINFO sfi;
			memset(&sfi, 0, sizeof(sfi));
			HIMAGELIST sysImageList = (HIMAGELIST)::SHGetFileInfo(fileExt.c_str(), FILE_ATTRIBUTE_NORMAL, &sfi,
																sizeof(SHFILEINFO), SHGFI_USEFILEATTRIBUTES|SHGFI_SYSICONINDEX);
			iconIndex = sfi.iIcon;
		}
		listCtrl.InsertItem(index++, nodeName.c_str(), iconIndex);
	}
}

// CzpEditorView diagnostics

#ifdef _DEBUG
void CzpEditorView::AssertValid() const
{
	CListView::AssertValid();
}

void CzpEditorView::Dump(CDumpContext& dc) const
{
	CListView::Dump(dc);
}

CzpEditorDoc* CzpEditorView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CzpEditorDoc)));
	return (CzpEditorDoc*)m_pDocument;
}
#endif //_DEBUG


// CzpEditorView message handlers
void CzpEditorView::OnStyleChanged(int nStyleType, LPSTYLESTRUCT lpStyleStruct)
{
	//TODO: add code to react to the user changing the view style of your window	
	CListView::OnStyleChanged(nStyleType,lpStyleStruct);	
}
