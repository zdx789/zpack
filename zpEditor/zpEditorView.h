
// zpEditorView.h : interface of the CzpEditorView class
//

#pragma once


class CzpEditorView : public CListView
{
protected: // create from serialization only
	CzpEditorView();
	DECLARE_DYNCREATE(CzpEditorView)

// Attributes
public:
	CzpEditorDoc* GetDocument() const;

// Operations
public:

// Overrides
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual void OnInitialUpdate(); // called first time after construct
// Implementation
public:
	virtual ~CzpEditorView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);

protected:
	afx_msg void OnStyleChanged(int nStyleType, LPSTYLESTRUCT lpStyleStruct);
	afx_msg void OnFilePrintPreview();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in zpEditorView.cpp
inline CzpEditorDoc* CzpEditorView::GetDocument() const
   { return reinterpret_cast<CzpEditorDoc*>(m_pDocument); }
#endif

