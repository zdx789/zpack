
// zpEditorDoc.cpp : implementation of the CzpEditorDoc class
//

#include "stdafx.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "zpEditor.h"
#endif

#include "zpEditorDoc.h"

#include <propkey.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CzpEditorDoc

IMPLEMENT_DYNCREATE(CzpEditorDoc, CDocument)

BEGIN_MESSAGE_MAP(CzpEditorDoc, CDocument)
	ON_COMMAND(ID_FILE_OPEN, &CzpEditorDoc::OnFileOpen)
	ON_COMMAND(ID_FILE_NEW, &CzpEditorDoc::OnFileNew)
	ON_COMMAND(ID_EDIT_ADD, &CzpEditorDoc::OnEditAdd)
	ON_COMMAND(ID_EDIT_DELETE, &CzpEditorDoc::OnEditDelete)
	ON_COMMAND(ID_EDIT_EXTRACT, &CzpEditorDoc::OnEditExtract)
	ON_COMMAND(ID_EDIT_DEFRAG, &CzpEditorDoc::OnEditDefrag)
	ON_COMMAND(ID_EDIT_ADD_FOLDER, &CzpEditorDoc::OnEditAddFolder)
END_MESSAGE_MAP()


// CzpEditorDoc construction/destruction

CzpEditorDoc::CzpEditorDoc()
{
	// TODO: add one-time construction code here

}

CzpEditorDoc::~CzpEditorDoc()
{
}

BOOL CzpEditorDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}




// CzpEditorDoc serialization

void CzpEditorDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

#ifdef SHARED_HANDLERS

// Support for thumbnails
void CzpEditorDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
{
	// Modify this code to draw the document's data
	dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

	CString strText = _T("TODO: implement thumbnail drawing here");
	LOGFONT lf;

	CFont* pDefaultGUIFont = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
	pDefaultGUIFont->GetLogFont(&lf);
	lf.lfHeight = 36;

	CFont fontDraw;
	fontDraw.CreateFontIndirect(&lf);

	CFont* pOldFont = dc.SelectObject(&fontDraw);
	dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
	dc.SelectObject(pOldFont);
}

// Support for Search Handlers
void CzpEditorDoc::InitializeSearchContent()
{
	CString strSearchContent;
	// Set search contents from document's data. 
	// The content parts should be separated by ";"

	// For example:  strSearchContent = _T("point;rectangle;circle;ole object;");
	SetSearchContent(strSearchContent);
}

void CzpEditorDoc::SetSearchContent(const CString& value)
{
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CMFCFilterChunkValueImpl *pChunk = NULL;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
		if (pChunk != NULL)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS

// CzpEditorDoc diagnostics

#ifdef _DEBUG
void CzpEditorDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CzpEditorDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CzpEditorDoc commands

ZpExplorer& CzpEditorDoc::GetZpExplorer()
{
	return m_explorer;
}

void CzpEditorDoc::OnFileOpen()
{
	CFileDialog dlg(TRUE, NULL, NULL, 0, "zpack files (*.zpk)|*.zpk|All Files (*.*)|*.*||");
	if (dlg.DoModal() != IDOK)
	{
		return;
	}
	CString filename = dlg.GetPathName();
	if (!m_explorer.open(filename.GetString()))
	{
		MessageBox(NULL, "Invalid zpack file.", "Error", MB_OK | MB_ICONERROR);
	}
	UpdateAllViews(NULL);
}

void CzpEditorDoc::OnFileNew()
{
	CFileDialog dlg(TRUE, NULL, NULL, 0, "zpack archives (*.zpk)|*.zpk|All Files (*.*)|*.*||");
	if (dlg.DoModal() != IDOK)
	{
		return;
	}
	CString filename = dlg.GetPathName();
	if (dlg.GetFileExt().IsEmpty())
	{
		filename += ".zpk";
	}
	if (!m_explorer.create(filename.GetString(), ""))
	{
		MessageBox(NULL, "Create package failed.", "Error", MB_OK | MB_ICONERROR);
	}
	UpdateAllViews(NULL);
}

void CzpEditorDoc::OnEditAdd()
{
	CFileDialog dlg(TRUE, NULL, NULL, OFN_ALLOWMULTISELECT);
	if (dlg.DoModal() != IDOK)
	{
		return;
	}
	POSITION pos = dlg.GetStartPosition();
	while (pos)
	{
		CString filename = dlg.GetNextPathName(pos);
		if (!m_explorer.add(filename.GetString(), ""))
		{
			MessageBox(NULL, "Add file failed", "Error", MB_OK | MB_ICONERROR);
		}
	}
	UpdateAllViews(NULL);
}

void CzpEditorDoc::OnEditAddFolder()
{
	//CFolderDialog dlg;
	//if( fdlg.DoModal() != IDOK )
}

void CzpEditorDoc::OnEditDelete()
{
}

void CzpEditorDoc::OnEditExtract()
{
}

void CzpEditorDoc::OnEditDefrag()
{
}

