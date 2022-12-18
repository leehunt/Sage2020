
// Sage2020Doc.h : interface of the CSage2020Doc class
//

#pragma once
#include <memory>
#include <vector>
#include "FileVersionDiff.h"
#include "FileVersionInstance.h"

class Sage2020ViewDocListener;

class CSage2020Doc : public CDocument {
 protected:  // create from serialization only
  CSage2020Doc() noexcept;
  DECLARE_DYNCREATE(CSage2020Doc)

  // Attributes
 public:
  FileVersionInstance* GetFileVersionInstance() const {
    return file_version_instance_ ? file_version_instance_.get() : nullptr;
  }

  const size_t GetFileVersionInstanceSize() const {
    if (!file_version_instance_)
      return 0;
    return file_version_instance_->GetLines().size();
  }

  const std::vector<FileVersionDiff>& GetRootFileDiffs() const {
    return file_diffs_;
  }

  Sage2020ViewDocListener* GetListenerHead() const {
    return m_pDocListenerHead;
  }

  // View state
  CPoint& viewport_origin() { return viewport_origin_; }

  // Operations
 public:
  // Overrides
 public:
  virtual BOOL OnNewDocument();
  virtual void Serialize(CArchive& ar);
#ifdef SHARED_HANDLERS
  virtual void InitializeSearchContent();
  virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
#endif  // SHARED_HANDLERS

  // Implementation
 public:
  virtual ~CSage2020Doc();
#ifdef _DEBUG
  virtual void AssertValid() const;
  virtual void Dump(CDumpContext& dc) const;
#endif

  void AddDocListener(Sage2020ViewDocListener& listener);
  void RemoveDocListener(Sage2020ViewDocListener& listener);

  afx_msg void OnUpdatePropertiesPaneGrid(CCmdUI* pCmdUI);
  afx_msg void OnUpdateHistoryTree(CCmdUI* pCmdUI);

 protected:
  // Generated message map functions
 protected:
  DECLARE_MESSAGE_MAP()

#ifdef SHARED_HANDLERS
  // Helper function that sets search content for a Search Handler
  void SetSearchContent(const CString& value);
#endif  // SHARED_HANDLERS

  std::vector<FileVersionDiff> file_diffs_;
  std::unique_ptr<FileVersionInstance> file_version_instance_;

  CPoint viewport_origin_;

  Sage2020ViewDocListener* m_pDocListenerHead;

  bool m_fNewDoc;

 public:
  virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
};
