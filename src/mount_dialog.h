#pragma once

#include "pch.h"
#include "ui_mount_dialog.h"

class MountDialog : public QDialog {
  Q_OBJECT

public:
  MountDialog(const QString &remote, const QDir &path,
              const QString &remoteType, const QString &remoteMode, QWidget *parent = nullptr);
  ~MountDialog();

  QString getMountPoint() const;
  bool isCheck() const;
  QStringList getOptions() const;

private:
  Ui::MountDialog ui;
  QString mTarget;

  void done(int r) override;

  QString mRemoteMode;
  QString mRemoteType;
  int mRcPort;

private slots:
   void shrink();

};
