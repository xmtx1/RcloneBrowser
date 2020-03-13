#include "mount_widget.h"
#include "utils.h"
#include "global.h"

MountWidget::MountWidget(QProcess *process, const QString &remote,
                         const QString &folder, const QStringList &args,
                         QWidget *parent)
    : QWidget(parent), mProcess(process) {
  ui.setupUi(this);

  mArgs.append(QDir::toNativeSeparators(GetRclone()));
  mArgs.append(args);

  QString info = QString("%1 on %2").arg(remote).arg(folder);
  QString infoTrimmed;

  if (info.length() > 140) {
    infoTrimmed = info.left(57) + "..." + info.right(80);
  } else {
    infoTrimmed = info;
  }

  ui.info->setText(infoTrimmed);
  ui.info->setCursorPosition(0);

  ui.remote->setText(remote);
  ui.remote->setCursorPosition(0);
  ui.remote->setToolTip(remote);

  ui.folder->setText(folder);
  ui.folder->setCursorPosition(0);
  ui.folder->setToolTip(folder);

  ui.details->setVisible(false);

  ui.output->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
  ui.output->setVisible(false);

  auto settings = GetSettings();
  QString iconsColour = settings->value("Settings/iconsColour").toString();

  QString img_add = "";

  if (iconsColour == "white") {
    img_add = "_inv";
  }

  ui.showDetails->setIcon(
      QIcon(":remotes/images/qbutton_icons/vrightarrow" + img_add + ".png"));
  ui.showDetails->setIconSize(QSize(24, 24));
  ui.showOutput->setIcon(
      QIcon(":remotes/images/qbutton_icons/vrightarrow" + img_add + ".png"));
  ui.showOutput->setIconSize(QSize(24, 24));

  QObject::connect(
      ui.showDetails, &QToolButton::toggled, this, [=](bool checked) {
        ui.details->setVisible(checked);
        if (checked) {
          ui.showDetails->setIcon(QIcon(
              ":remotes/images/qbutton_icons/vdownarrow" + img_add + ".png"));
          ui.showDetails->setIconSize(QSize(24, 24));
        } else {
          ui.showDetails->setIcon(QIcon(
              ":remotes/images/qbutton_icons/vrightarrow" + img_add + ".png"));
          ui.showDetails->setIconSize(QSize(24, 24));
        }
      });

  ui.copy->setIcon(
      QIcon(":remotes/images/qbutton_icons/copy" + img_add + ".png"));
  ui.copy->setIconSize(QSize(24, 24));

  QObject::connect(ui.copy, &QToolButton::clicked, this, [=]() {
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(mArgs.join(" "));
  });

  QObject::connect(
      ui.showOutput, &QToolButton::toggled, this, [=](bool checked) {
        ui.output->setVisible(checked);
        if (checked) {
          ui.showOutput->setIcon(QIcon(
              ":remotes/images/qbutton_icons/vdownarrow" + img_add + ".png"));
          ui.showOutput->setIconSize(QSize(24, 24));
        } else {
          ui.showOutput->setIcon(QIcon(
              ":remotes/images/qbutton_icons/vrightarrow" + img_add + ".png"));
          ui.showOutput->setIconSize(QSize(24, 24));
        }
      });

  ui.cancel->setIcon(
      QIcon(":remotes/images/qbutton_icons/cancel" + img_add + ".png"));
  ui.cancel->setIconSize(QSize(24, 24));

  QObject::connect(ui.cancel, &QToolButton::clicked, this, [=]() {
    if (isRunning) {
      int button = QMessageBox::question(
          this, "Unmount",
          QString("Do you want to unmount?\n\n %2 \n\n mounted to \n\n %1").arg(folder).arg(remote),
          QMessageBox::Yes | QMessageBox::No);
      if (button == QMessageBox::Yes) {
        cancel();
      }
    } else {

      emit closed();
    }
  });

  QObject::connect(mProcess, &QProcess::readyRead, this, [=]() {
    while (mProcess->canReadLine()) {
      ui.output->appendPlainText(mProcess->readLine().trimmed());
    }
  });

  QObject::connect(
      mProcess,
      static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(
          &QProcess::finished),
      this, [=](int status, QProcess::ExitStatus) {
        mProcess->deleteLater();
        isRunning = false;

        QString info = "Mounted " + ui.info->text();
        QString infoTrimmed;
        if (info.length() > 140) {
          infoTrimmed = info.left(57) + "..." + info.right(80);
        } else {
          infoTrimmed = info;
        }
        ui.info->setText(infoTrimmed);
        ui.info->setCursorPosition(0);

        if (status == 0) {
          if (iconsColour == "white") {
            ui.showDetails->setStyleSheet(
                "QToolButton { border: 0; font-weight: normal;}");
          } else {
            ui.showDetails->setStyleSheet(
                "QToolButton { border: 0; color: black; font-weight: normal;}");
          }
          ui.showDetails->setText("  Finished");
        } else {
          ui.showDetails->setStyleSheet(
              "QToolButton { border: 0; color: red; }");
          ui.showDetails->setText("  Error");
        }
        ui.cancel->setToolTip("Close");
        ui.cancel->setStatusTip("Close");
        emit finished();
      });

  ui.showDetails->setStyleSheet(
      "QToolButton { border: 0; color: green; font-weight: bold;}");
  ui.showDetails->setText("  Mounted");
}

MountWidget::~MountWidget() {}

void MountWidget::cancel() {
  if (!isRunning) {
    return;
  }

  QString cmd;

#if defined(Q_OS_MACOS) || defined(Q_OS_FREEBSD)
  QProcess::startDetached("umount", QStringList() << ui.folder->text());
#else
#if defined(Q_OS_WIN32)
  QProcess *p = new QProcess();
  QStringList args;
  args << "rc";

  // requires rlone version at least 1.50
  args << "core/quit";

  args << "--rc-addr";

  // get RC parameters from mArgs
  int index = 0;
  QString localhost;
  QString user;
  QString password;
  int rcPort;
  QString rcPortString;

  index = mArgs.indexOf(QRegExp("^localhost:\\d+$"));
  localhost = mArgs.at(index);
  rcPortString = localhost;
  rcPort = rcPortString.replace("localhost:","").toInt();

  index = mArgs.indexOf(QRegExp("^--rc-user\\S+"));
  user =  mArgs.at(index);

  index = mArgs.indexOf(QRegExp("^--rc-pass\\S+"));
  password = mArgs.at(index);

  args << localhost << user << password;

  UseRclonePassword(p);
  p->start(GetRclone(), args, QIODevice::ReadOnly);

/*
  //!!! remove rcPort from global list
  if ( global.usedRcPorts.contains(rcPort) ) {
    global.usedRcPorts.removeOne(rcPort);
  }
*/

#else
  QProcess::startDetached("fusermount", QStringList()
                                            << "-u" << ui.folder->text());
#endif
#endif

  mProcess->waitForFinished();

  auto settings = GetSettings();
  QString iconsColour = settings->value("Settings/iconsColour").toString();

  if (iconsColour == "white") {
    ui.showDetails->setStyleSheet(
        "QToolButton { border: 0; font-weight: normal; }");
  } else {
    ui.showDetails->setStyleSheet(
        "QToolButton { border: 0; color: black; font-weight: normal;}");
  }

  ui.showDetails->setText("  Finished");
  ui.cancel->setToolTip("Close");
  ui.cancel->setStatusTip("Close");
}
