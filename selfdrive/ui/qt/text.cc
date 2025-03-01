#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QWidget>
#include <QProcess>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QAbstractSocket>

#include "selfdrive/hardware/hw.h"
#include "selfdrive/ui/qt/util.h"
#include "selfdrive/ui/qt/qt_window.h"
#include "selfdrive/ui/qt/widgets/scrollview.h"
#include "selfdrive/ui/qt/widgets/input.h"
#include "selfdrive/common/params.h"

int main(int argc, char *argv[]) {
  initApp();
  QApplication a(argc, argv);
  QWidget window;
  setMainWindow(&window);

  QGridLayout *main_layout = new QGridLayout(&window);
  main_layout->setMargin(50);

  QLabel *label = new QLabel(argv[1]);
  label->setWordWrap(true);
  label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
  ScrollView *scroll = new ScrollView(label);
  scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  main_layout->addWidget(scroll, 0, 0, Qt::AlignTop);

  // Scroll to the bottom
  QObject::connect(scroll->verticalScrollBar(), &QAbstractSlider::rangeChanged, [=]() {
    scroll->verticalScrollBar()->setValue(scroll->verticalScrollBar()->maximum());
  });

  QPushButton *btn = new QPushButton();
#ifdef __aarch64__
  QPushButton *btn2 = new QPushButton();
  QPushButton *btn3 = new QPushButton();
  QLabel *label2 = new QLabel();
  QString device_ip = "---";
  const QHostAddress &localhost = QHostAddress(QHostAddress::LocalHost);
  for (const QHostAddress &address: QNetworkInterface::allAddresses()) {
    if (address.protocol() == QAbstractSocket::IPv4Protocol && address != localhost)
      device_ip = address.toString();
  }
  label2->setText(device_ip);
  label2->setStyleSheet("color: #e0e879");
  main_layout->addWidget(label2, 0, 0, Qt::AlignRight | Qt::AlignTop);
  btn->setText(" Update ");
  btn2->setText("MixPlorer");
  btn3->setText(" Restore ");
  QObject::connect(btn, &QPushButton::clicked, [=]() {
    QProcess::execute("pkill -f thermald");
    QProcess::execute("rm -f /data/openpilot/prebuilt");
    QProcess::execute("/data/openpilot/selfdrive/assets/addon/script/gitpull.sh");
    QProcess::execute("reboot");
  });
  QObject::connect(btn2, &QPushButton::clicked, [=]() {
    btn2->setEnabled(false);
    QProcess::execute("/data/openpilot/selfdrive/assets/addon/script/run_mixplorer.sh");
  });
  QObject::connect(btn3, &QPushButton::clicked, [=]() {
    btn3->setEnabled(false);
    QString cmd = "git reset --hard " + QString::fromStdString(Params().get("GitCommit"));
    QProcess::execute(cmd);
    QProcess::execute("reboot");
  });
  main_layout->addWidget(btn2, 1, 0, Qt::AlignLeft | Qt::AlignBottom);
  main_layout->addWidget(btn3, 1, 0, Qt::AlignCenter | Qt::AlignBottom);
#else
  btn->setText("Exit");
  QObject::connect(btn, &QPushButton::clicked, &a, &QApplication::quit);
#endif
  main_layout->addWidget(btn, 1, 0, Qt::AlignRight | Qt::AlignBottom);

  window.setStyleSheet(R"(
    * {
      outline: none;
      color: white;
      background-color: black;
      font-size: 50px;
    }
    QPushButton {
      padding: 40px;
      padding-right: 80px;
      padding-left: 80px;
      border: 2px solid white;
      border-radius: 20px;
      margin-right: 30px;
    }
  )");

  return a.exec();
}
