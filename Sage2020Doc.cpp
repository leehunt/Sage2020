
// Sage2020Doc.cpp : implementation of the CSage2020Doc class
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "Sage2020.h"
#endif

#include <codecvt>
#include "GitDiffReader.h"
#include "GitFileReader.h"
#include <propkey.h>
#include "Sage2020Doc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CSage2020Doc

IMPLEMENT_DYNCREATE(CSage2020Doc, CDocument)

BEGIN_MESSAGE_MAP(CSage2020Doc, CDocument)
END_MESSAGE_MAP()


// CSage2020Doc construction/destruction

CSage2020Doc::CSage2020Doc() noexcept
{
	// TODO: add one-time construction code here

}

CSage2020Doc::~CSage2020Doc()
{
}

BOOL CSage2020Doc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}




// CSage2020Doc serialization

void CSage2020Doc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
    auto path = ar.GetFile()->GetFilePath();
    std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
    GitDiffReader git_diff_reader{myconv.to_bytes(path)};
    file_diffs_ = git_diff_reader.GetDiffs();

		if (file_diffs_.size() > 0) {
      GitFileReader git_file_reader{file_diffs_[0].diff_tree_.new_hash_string};
      int commit_sequence_num = static_cast<int>(file_diffs_.size());
			file_version_instance_ =
                            std::make_unique<FileVersionInstance>(
                                std::move(git_file_reader.GetLines()),
                                commit_sequence_num);
    }
	}
}

#ifdef SHARED_HANDLERS

// Support for thumbnails
void CSage2020Doc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
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
void CSage2020Doc::InitializeSearchContent()
{
	CString strSearchContent;
	// Set search contents from document's data.
	// The content parts should be separated by ";"

	// For example:  strSearchContent = _T("point;rectangle;circle;ole object;");
	SetSearchContent(strSearchContent);
}

void CSage2020Doc::SetSearchContent(const CString& value)
{
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CMFCFilterChunkValueImpl *pChunk = nullptr;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
		if (pChunk != nullptr)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS

// CSage2020Doc diagnostics

#ifdef _DEBUG
void CSage2020Doc::AssertValid() const
{
	CDocument::AssertValid();
}

void CSage2020Doc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CSage2020Doc commands
