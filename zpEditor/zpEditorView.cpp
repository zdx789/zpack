
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
#include "FolderDialog.h"
#include <sstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CzpEditorView

IMPLEMENT_DYNCREATE(CzpEditorView, CListView)

BEGIN_MESSAGE_MAP(CzpEditorView, CListView)
	ON_WM_STYLECHANGED()
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONUP()
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnDbClick)
	ON_WM_KEYDOWN()
	ON_COMMAND(ID_EDIT_ADD_FOLDER, &CzpEditorView::OnEditAddFolder)
	ON_COMMAND(ID_EDIT_ADD, &CzpEditorView::OnEditAdd)
	ON_COMMAND(ID_EDIT_DELETE, &CzpEditorView::OnEditDelete)
	ON_COMMAND(ID_EDIT_EXTRACT, &CzpEditorView::OnEditExtract)
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

	listCtrl.ModifyStyle(0, LVS_REPORT);
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
	//HMENU contextMenu = theApp.GetContextMenuManager()->GetMenuById(IDR_POPUP_EDIT);
	//::EnableMenuItem(contextMenu, 0, MF_GRAYED | MF_BYPOSITION);
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
	if (currentNode != explorer.rootNode())
	{
		listCtrl.InsertItem(index++, "..", 3);
	}
	for (std::list<ZpNode>::const_iterator iter = currentNode->children.cbegin();
		iter != currentNode->children.cend();
		++iter)
	{
		const ZpNode& node = *iter;
		const std::string& nodeName = node.name;
		int iconIndex = 3;	//3 suppose to be folder icon
		if (!node.isDirectory)
		{
			std::string fileExt;
			size_t pos = nodeName.find_last_of('.');
			if (pos == std::string::npos)
			{
				fileExt = "holycrap";	//can not be empty, or will get some icon like your c:
			}
			else
			{
				fileExt = nodeName.substr(pos, nodeName.length() - pos);
			}
			SHFILEINFO sfi;
			memset(&sfi, 0, sizeof(sfi));
			HIMAGELIST sysImageList = (HIMAGELIST)::SHGetFileInfo(fileExt.c_str(), FILE_ATTRIBUTE_NORMAL, &sfi,
																sizeof(SHFILEINFO), SHGFI_USEFILEATTRIBUTES|SHGFI_SYSICONINDEX);
			iconIndex = sfi.iIcon;
		}
		std::ostringstream sizeString;
		if (!node.isDirectory)
		{
			sizeString << iter->fileSize;
		}
		listCtrl.InsertItem(index, nodeName.c_str(), iconIndex);
		listCtrl.SetItemText(index, 1, sizeString.str().c_str());
		listCtrl.SetItemData(index, (DWORD_PTR)&node);
		++index;
	}
}

void CzpEditorView::OnDbClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	CListCtrl& listCtrl = GetListCtrl();
	
	LPNMITEMACTIVATE item = (LPNMITEMACTIVATE) pNMHDR;
	int selected = item->iItem;

	if (selected < 0 && selected >= listCtrl.GetItemCount())
	{
		return;
	}
	ZpExplorer& explorer = GetDocument()->GetZpExplorer();
	ZpNode* node = (ZpNode*)listCtrl.GetItemData(selected);
	if (node == NULL)
	{
		explorer.enterDir("..");
	}
	else if (node->isDirectory)
	{
		explorer.setCurrentNode(node);
	}
	else
	{
		return;
	}
	m_pDocument->UpdateAllViews(NULL);
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
	CListView::OnStyleChanged(nStyleType,lpStyleStruct);	
}

void CzpEditorView::OnEditDelete()
{
	ZpExplorer& explorer = GetDocument()->GetZpExplorer();
	CListCtrl& listCtrl = GetListCtrl();

	bool anythingRemoved = false;
	POSITION pos = listCtrl.GetFirstSelectedItemPosition();
	while (pos != NULL)
	{
		int selectedIndex = listCtrl.GetNextSelectedItem(pos);
		ZpNode* node = (ZpNode*)listCtrl.GetItemData(selectedIndex);
		if (node == NULL)
		{
			continue;
		}
		if (!anythingRemoved)
		{
			std::string warning = "Do you want to delete ";
			if (pos == NULL)	//only one
			{
				warning += "\"";
				warning += node->name;
				warning += "\"";
			}
			else
			{
				warning += "seleted files/directories";
			}
			warning += "?";
			if (::MessageBox(NULL, warning.c_str(), "Question", MB_YESNO | MB_ICONQUESTION) != IDYES)
			{
				return;
			}
		}
		explorer.remove(node->name);
		anythingRemoved = true;
	}
	if (anythingRemoved)
	{
		m_pDocument->UpdateAllViews(NULL);
	}
}

void CzpEditorView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == VK_DELETE)
	{
		OnEditDelete();
	}
	else if (nChar == VK_RETURN)
	{

	}
}

void CzpEditorView::OnEditAddFolder()
{
	CFolderDialog dlg(NULL, "Select folder to add to package.");
	if (dlg.DoModal() != IDOK)
	{
		return;
	}
	std::string addToDir;
	ZpNode* node = getSelectedNode();
	if (node != NULL)
	{
		addToDir = node->name;
	}
	CString path = dlg.GetPathName();
	ZpExplorer& explorer = GetDocument()->GetZpExplorer();
	if (!explorer.add(path.GetString(), addToDir.c_str()))
	{
		::MessageBox(NULL, "Add folder failed.", "Error", MB_OK | MB_ICONERROR);
		return;
	}
	m_pDocument->UpdateAllViews(NULL);
}

void CzpEditorView::OnEditAdd()
{
	CFileDialog dlg(TRUE, NULL, NULL, OFN_ALLOWMULTISELECT);
	if (dlg.DoModal() != IDOK)
	{
		return;
	}
	std::string addToDir;
	ZpNode* node = getSelectedNode();
	if (node != NULL)
	{
		addToDir = node->name;
	}
	ZpExplorer& explorer = GetDocument()->GetZpExplorer();
	POSITION pos = dlg.GetStartPosition();
	while (pos)
	{
		CString filename = dlg.GetNextPathName(pos);
		if (!explorer.add(filename.GetString(), addToDir.c_str()))
		{
			::MessageBox(NULL, "Add file failed", "Error", MB_OK | MB_ICONERROR);
		}
	}
	m_pDocument->UpdateAllViews(NULL);
}

void CzpEditorView::OnEditExtract()
{
	CFolderDialog dlg(NULL, "Select dest folder to extract.");
	if (dlg.DoModal() != IDOK)
	{
		return;
	}
	std::string destPath = dlg.GetPathName().GetString();

	ZpExplorer& explorer = GetDocument()->GetZpExplorer();
	CListCtrl& listCtrl = GetListCtrl();
	POSITION pos = listCtrl.GetFirstSelectedItemPosition();
	if (pos == NULL)
	{
		//nothing selected, extract current folder
		explorer.extract(".", destPath);
		return;
	}
	while (pos != NULL)
	{
		int selectedIndex = listCtrl.GetNextSelectedItem(pos);
		ZpNode* node = (ZpNode*)listCtrl.GetItemData(selectedIndex);
		if (node == NULL)
		{
			continue;
		}
		explorer.extract(node->name, destPath);
	}
}

ZpNode* CzpEditorView::getSelectedNode()
{
	CListCtrl& listCtrl = GetListCtrl();
	POSITION selectedPos = listCtrl.GetFirstSelectedItemPosition();
	if (selectedPos != NULL)
	{
		int selectedIndex = listCtrl.GetNextSelectedItem(selectedPos);
		return (ZpNode*)listCtrl.GetItemData(selectedIndex);
	}
	return NULL;
}