
// LeftView.cpp : implementation of the CLeftView class
//

#include "stdafx.h"
#include "zpEditor.h"

#include "zpEditorDoc.h"
#include "LeftView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CLeftView

IMPLEMENT_DYNCREATE(CLeftView, CTreeView)

BEGIN_MESSAGE_MAP(CLeftView, CTreeView)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelectChanged)
END_MESSAGE_MAP()


// CLeftView construction/destruction

CLeftView::CLeftView()
{
	// TODO: add construction code here
}

CLeftView::~CLeftView()
{
}

BOOL CLeftView::PreCreateWindow(CREATESTRUCT& cs)
{
	return CTreeView::PreCreateWindow(cs);
}

void CLeftView::OnInitialUpdate()
{
	CTreeView::OnInitialUpdate();

	CTreeCtrl& treeCtrl = GetTreeCtrl();
	treeCtrl.ModifyStyle(0, TVS_HASBUTTONS | TVS_HASLINES);

	CBitmap bm;
	bm.LoadBitmap(IDB_BITMAP_FOLDER);
	m_imageList.Create(16, 16, ILC_COLOR24 | ILC_MASK, 0, 0);
	m_imageList.Add(&bm, RGB(255, 255, 255));
	treeCtrl.SetImageList(&m_imageList, TVSIL_NORMAL);
}

void CLeftView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	const ZpExplorer& explorer = GetDocument()->GetZpExplorer();
	const ZpNode* node = explorer.currentNode();
	HTREEITEM currentItem = (HTREEITEM)node->userData;
	CTreeCtrl& treeCtrl = GetTreeCtrl();
	if (currentItem != NULL)
	{
		updateNode(currentItem);
		treeCtrl.SelectItem(currentItem);
		return;
	}
	
	treeCtrl.DeleteAllItems();

	const ZpNode* rootNode = explorer.rootNode();
	if (!explorer.isOpen())
	{
		return;
	}
	std::string packName;
	const std::string& packFilename = explorer.packageFilename();
	size_t slashPos = packFilename.find_last_of('\\');
	if (slashPos == std::string::npos)
	{
		packName = packFilename;
	}
	else
	{
		packName = packFilename.substr(slashPos + 1, packFilename.length() - slashPos - 1);
	}
	HTREEITEM rootItem = treeCtrl.InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE | TVIF_PARAM,
										packName.c_str(),
										0,
										1,
										TVIS_EXPANDED | INDEXTOSTATEIMAGEMASK(1),
										TVIS_EXPANDED | TVIS_STATEIMAGEMASK,
										(LPARAM)rootNode,
										TVI_ROOT,
										TVI_LAST);
	rootNode->userData = rootItem;

	updateNode(rootItem);

	treeCtrl.Select(rootItem, TVGN_CARET);
}

void CLeftView::updateNode(HTREEITEM ti)
{
	CTreeCtrl& treeCtrl = GetTreeCtrl();
	//delete old children
	if (treeCtrl.ItemHasChildren(ti))
	{
		HTREEITEM childItem = treeCtrl.GetChildItem(ti);
		while (childItem != NULL)
		{
			HTREEITEM nextItem = treeCtrl.GetNextItem(childItem, TVGN_NEXT);
			treeCtrl.DeleteItem(childItem);
			childItem = nextItem;
		}
	}
	//add new children
	const ZpNode* node = (const ZpNode*)treeCtrl.GetItemData(ti);
	for (std::list<ZpNode>::const_iterator iter = node->children.cbegin();
		iter != node->children.cend();
		++iter)
	{
		const ZpNode& child = *iter;
		if (!child.isDirectory)
		{
			continue;
		}
		HTREEITEM childItem = treeCtrl.InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM,
												child.name.c_str(),
												0,
												1,
												TVIS_EXPANDED | INDEXTOSTATEIMAGEMASK(1),
												TVIS_EXPANDED | TVIS_STATEIMAGEMASK,
												(LPARAM)&child,
												ti,
												TVI_LAST);
		child.userData = childItem;
		updateNode(childItem);
	}
}

void CLeftView::OnSelectChanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	CTreeCtrl& treeCtrl = GetTreeCtrl();
	HTREEITEM selectedItem = treeCtrl.GetSelectedItem();
	if (selectedItem != NULL)
	{
		const ZpNode* selectedNode = (const ZpNode*)treeCtrl.GetItemData(selectedItem);
		ZpExplorer& explorer = GetDocument()->GetZpExplorer();
		explorer.setCurrentNode(selectedNode);
		m_pDocument->UpdateAllViews(this);
	}
}

// CLeftView diagnostics

#ifdef _DEBUG
void CLeftView::AssertValid() const
{
	CTreeView::AssertValid();
}

void CLeftView::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);
}

CzpEditorDoc* CLeftView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CzpEditorDoc)));
	return (CzpEditorDoc*)m_pDocument;
}
#endif //_DEBUG


// CLeftView message handlers
