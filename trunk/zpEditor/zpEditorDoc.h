
// zpEditorDoc.h : interface of the CzpEditorDoc class
//
#pragma once

#include "zpExplorer.h"

class CzpEditorDoc : public CDocument
{
protected: // create from serialization only
	CzpEditorDoc();
	DECLARE_DYNCREATE(CzpEditorDoc)

// Attributes
public:
	ZpExplorer& GetZpExplorer();

public:

// Overrides
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
#ifdef SHARED_HANDLERS
	virtual void InitializeSearchContent();
	virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
#endif // SHARED_HANDLERS

// Implementation
public:
	virtual ~CzpEditorDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()

#ifdef SHARED_HANDLERS
	// Helper function that sets search content for a Search Handler
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS

public:
	afx_msg void OnFileOpen();

private:
	ZpExplorer	m_explorer;
public:
	afx_msg void OnFileNew();
	afx_msg void OnEditAdd();
	afx_msg void OnEditDelete();
	afx_msg void OnEditExtract();
	afx_msg void OnEditDefrag();
	afx_msg void OnEditAddFolder();
};
