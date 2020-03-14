#include "mount_dialog.h"
#include "global.h"
#include "utils.h"

MountDialog::MountDialog(const QString &remote, const QDir &path,
                         const QString &remoteType, const QString &remoteMode,
                         QWidget *parent)
    : QDialog(parent) {

  ui.setupUi(this);

  mRemoteMode = remoteMode;
  mRemoteType = remoteType;

  auto settings = GetSettings();

  // set minimumWidth based on font size
  int fontsize = 0;
  fontsize = (settings->value("Settings/fontSize").toInt());
  setMinimumWidth(minimumWidth() + (fontsize * 30));
  QTimer::singleShot(0, this, SLOT(shrink()));

#if !defined(Q_OS_WIN)
  ui.groupBox_Win->hide();
  QTimer::singleShot(0, this, SLOT(shrink()));
#endif

  //!!!Windows code
#if defined(Q_OS_WIN)
  ui.groupBox_notWin->hide();
  QTimer::singleShot(0, this, SLOT(shrink()));

//!!!
  for (const auto &i : global.usedRcPorts) {
    qDebug() << "element: " << i;
  }

  // get rc port
  int rcPortStartWin = (settings->value("Settings/rcPortStartWin").toInt());

  int i = 0;
  bool isFree = true;
  int mRcPort = 0;

  // is this on the list already used ports? - global.usedRcPorts on list
  // arbitrary value but in real life should be enough
  while (i < 256) {

    isFree = true;

    if (global.usedRcPorts.contains(rcPortStartWin + i)) {
      isFree = false;
    }

    if (isFree) {
      // !!! we should check if port actually free
      mRcPort = rcPortStartWin + i;
      break;
    }
    ++i;
  }

  // set initial mount point option to drive letter
  ui.le_rcPort->setText(QString::number(mRcPort));

  ui.label_mountBase->setDisabled(true);
  ui.le_mountBase->setDisabled(true);
  ui.le_mountBaseBrowse->setDisabled(true);
  ui.label_mountPointWin->setDisabled(true);
  ui.le_mountPointWin->setDisabled(true);
  ui.label_mountPointWinInfo->setDisabled(true);
  ui.label_driveLetter->setDisabled(false);
  ui.combo_driveLetter->setDisabled(false);
  ui.label_driveLetterInfo->setDisabled(false);

  // initailize drive letters

  QStringList drivesList;
  for( char l = 'A'; l<='Z'; ++l) {
    drivesList << QString(l);
  }

  // get used drives' letters
  QStringList disksUsed;

  // get mounted drives (missing CD ROM but gets mounts)
  foreach (QFileInfo drive, QDir::drives()) {
    if (!QString(drive.absolutePath().front()).isEmpty()) {
      disksUsed << QString(drive.absolutePath().front());
    }
  }
  // get all local storage drives (catches CD ROM but missing mounts)
  foreach (const QStorageInfo &storage, QStorageInfo::mountedVolumes()) {
        if (storage.isValid()) {
           disksUsed << QString(storage.rootPath().front());
        }
   }
  // remove disksUsed from DrivesList
  for (const auto &i : disksUsed) {
    if (drivesList.contains(i)) {
      drivesList.removeOne(i);
    }
  }

  ui.combo_driveLetter->addItems(drivesList);
#endif

  setWindowTitle("Mount remote");

  // set combo box tooltips
  ui.combob_cacheLevel->setItemData(0, "default", Qt::ToolTipRole);
  ui.combob_cacheLevel->setItemData(1, "--vfs-cache-mode minimal",
                                    Qt::ToolTipRole);
  ui.combob_cacheLevel->setItemData(2, "--vfs-cache-mode writes",
                                    Qt::ToolTipRole);
  ui.combob_cacheLevel->setItemData(3, "--vfs-cache-mode full",
                                    Qt::ToolTipRole);

  ui.le_rcPort->setValidator(new QIntValidator(49152, 65535, this));

  mTarget = remote + ":" + path.path();

  ui.remoteToCheck->setText(mTarget);
  ui.remoteToCheck->setCursorPosition(0);

  QObject::connect(ui.buttonBox, &QDialogButtonBox::accepted, this,
                   &QDialog::accept);

  QObject::connect(ui.buttonBox, &QDialogButtonBox::rejected, this,
                   &QDialog::reject);

  QObject::connect(ui.cb_driveLetter, &QCheckBox::clicked, this, [=]() {
    if (ui.cb_driveLetter->isChecked()) {

      ui.label_mountBase->setDisabled(true);
      ui.le_mountBase->setDisabled(true);
      ui.le_mountBaseBrowse->setDisabled(true);
      ui.label_mountPointWin->setDisabled(true);
      ui.le_mountPointWin->setDisabled(true);
      ui.label_mountPointWinInfo->setDisabled(true);

      ui.label_driveLetter->setDisabled(false);
      ui.combo_driveLetter->setDisabled(false);
      ui.label_driveLetterInfo->setDisabled(false);

    } else {
    }
    //    ui.sizeOnly->setDisabled(false);
  });

  QObject::connect(ui.cb_mountPointWin, &QCheckBox::clicked, this, [=]() {
    if (ui.cb_mountPointWin->isChecked()) {

      ui.label_mountBase->setDisabled(false);
      ui.le_mountBase->setDisabled(false);
      ui.le_mountBaseBrowse->setDisabled(false);
      ui.label_mountPointWin->setDisabled(false);
      ui.le_mountPointWin->setDisabled(false);
      ui.label_mountPointWinInfo->setDisabled(false);

      ui.label_driveLetter->setDisabled(true);
      ui.combo_driveLetter->setDisabled(true);
      ui.label_driveLetterInfo->setDisabled(true);

    } else {
    }
    //    ui.download->setDisabled(false);
  });

  QObject::connect(ui.le_mountBaseBrowse, &QToolButton::clicked, this, [=]() {
    QString sourceDir =
        QFileDialog::getExistingDirectory(this, "Choose mounting base folder");
    if (!sourceDir.isEmpty()) {
      ui.le_mountBase->setText(QDir::toNativeSeparators(sourceDir));
      ui.le_mountBase->setCursorPosition(0);
    }
  });

  QObject::connect(ui.tb_mountPointNotWin, &QToolButton::clicked, this, [=]() {
    QString sourceDir =
        QFileDialog::getExistingDirectory(this, "Choose mounting point");
    if (!sourceDir.isEmpty()) {
      ui.le_mountPointNotWin->setText(QDir::toNativeSeparators(sourceDir));
      ui.le_mountPointNotWin->setCursorPosition(0);
    }
  });
}

MountDialog::~MountDialog() {}

void MountDialog::shrink() {
  resize(0, 0);
  adjustSize();
  setMaximumHeight(this->height());
}


#if !defined(Q_OS_WIN)
QString MountDialog::getMountPoint() const {
  return ui.le_mountPointNotWin->text();
}
#endif

#if defined(Q_OS_WIN)
QString MountDialog::getMountPoint() const {
  QString mountPointWindows;
  if (ui.cb_driveLetter->isChecked()) {
    // drive letter
    mountPointWindows = ui.combo_driveLetter->currentText() + ":";
  } else {
    // base/mountpoint
    mountPointWindows =  ui.le_mountBase->text().trimmed() + '\\' + ui.le_mountPointWin->text().trimmed();
  }
  return mountPointWindows;
}
#endif


bool MountDialog::isCheck() const { return ui.cb_driveLetter->isChecked(); }

QStringList MountDialog::getOptions() const {

  QStringList list;

  list << "mount";

  list << mTarget;

#if !defined(Q_OS_WIN)
  list << ui.le_mountPointNotWin->text();
#endif

  //!!!Windows code
#if defined(Q_OS_WIN)
  if (ui.cb_driveLetter->isChecked()) {
    // drive letter
    list << ui.combo_driveLetter->currentText() + ":";
  } else {
    // base/mountpoint
    list << ui.le_mountBase->text().trimmed() + '\\' +
                ui.le_mountPointWin->text().trimmed();
  }
#endif

  if (!ui.le_rcPort->text().isEmpty()) {

    list << "--rc";
    list << "--rc-addr";
    list << "localhost:" + QVariant(ui.le_rcPort->text().toInt()).toString();

    // generate random username and password
    const QString possibleCharacters(
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");

    QString rcUser;
    for (int i = 0; i < 10; ++i) {
      int index = qrand() % possibleCharacters.length();
      QChar nextChar = possibleCharacters.at(index);
      rcUser.append(nextChar);
    }

    QString rcPass;
    for (int i = 0; i < 22; ++i) {
      int index = qrand() % possibleCharacters.length();
      QChar nextChar = possibleCharacters.at(index);
      rcPass.append(nextChar);
    }

    list << "--rc-user=" + rcUser;
    list << "--rc-pass=" + rcPass;
  }

  if (mRemoteType == "drive") {

    if (mRemoteMode == "shared") {
      list << "--drive-shared-with-me";
      if (!ui.cb_readOnly->isChecked()) {
        list << "--read-only";
      }
    }

    if (mRemoteMode == "trash") {
      list << "--drive-trashed-only";
    }
  }

  if (ui.cb_readOnly->isChecked()) {
    list << "--read-only";
  }

  if (!(ui.le_volumeName->text().trimmed().isEmpty())) {
    list << "--volname";
    list << ui.le_volumeName->text().trimmed();
  }

  switch (ui.combob_cacheLevel->currentIndex()) {
  case 0:
    break;
  case 1:
    list << "--vfs-cache-mode";
    list << "minimal";
    break;
  case 2:
    list << "--vfs-cache-mode";
    list << "writes";
    break;
  case 3:
    list << "--vfs-cache-mode";
    list << "full";
    break;
  }

  if (!ui.textExtra->toPlainText().trimmed().isEmpty()) {

    for (auto line : ui.textExtra->toPlainText().trimmed().split('\n')) {
      if (!line.isEmpty()) {

        for (QString arg :
             line.split(QRegExp(" (?=[^\"]*(\"[^\"]*\"[^\"]*)*$)"))) {
          if (!arg.isEmpty()) {
            list << arg.replace("\"", "");
          }
        }
      }
    }
  }

  return list;
}

void MountDialog::done(int r) {
  if (r == QDialog::Accepted) {

#if !defined(Q_OS_WIN)
    if (ui.le_mountPointNotWin->text().trimmed().isEmpty()) {
      QMessageBox::warning(this, "Warning",
                           "Please enter mount point directory!");
      return;
    }
#endif

// only on Windows RC mode is mandatory (for unmount to work)
#if !defined(Q_OS_WIN)
    if (!ui.le_rcPort->text().trimmed().isEmpty()) {
      if (ui.le_rcPort->text().trimmed().toInt() < 1024 ||
          ui.le_rcPort->text().trimmed().toInt() > 65535) {
        QMessageBox::warning(
            this, "Warning",
            "RC port has to be between 1024 and 65535!\n\n Or leave it empty.");
        return;
      }
    }
#endif

#if defined(Q_OS_WIN)
    if (ui.le_rcPort->text().trimmed().toInt() < 1024 ||
        ui.le_rcPort->text().trimmed().toInt() > 65535 ||
        ui.le_rcPort->text().trimmed().isEmpty()) {
      QMessageBox::warning(this, "Warning",
                           "RC port has to be between 1024 and 65535!");
      return;
    }

    if (!ui.cb_driveLetter->isChecked()) {

      if (ui.le_mountBase->text().trimmed().isEmpty() ||
          ui.le_mountPointWin->text().trimmed().isEmpty()) {

        QMessageBox::warning(
            this, "Warning",
            "Please enter mount base and mount point directories!");
        return;
      }
    }

#endif
  }

  QDialog::done(r);
}
