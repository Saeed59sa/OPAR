#include "selfdrive/ui/qt/widgets/input.h"

#include <QPushButton>
#include <QProcess>

#include "selfdrive/hardware/hw.h"
#include "selfdrive/ui/qt/util.h"
#include "selfdrive/ui/qt/qt_window.h"
#include "selfdrive/ui/qt/widgets/scrollview.h"


QDialogBase::QDialogBase(QWidget *parent) : QDialog(parent) {
  Q_ASSERT(parent != nullptr);
  parent->installEventFilter(this);

  setStyleSheet(R"(
    * {
      outline: none;
      color: white;
      font-family: Inter;
    }
    QDialogBase {
      background-color: black;
    }
    QPushButton {
      height: 160;
      font-size: 55px;
      font-weight: 400;
      border-radius: 10px;
      color: white;
      background-color: #333333;
    }
    QPushButton:pressed {
      background-color: #444444;
    }
  )");
}

bool QDialogBase::eventFilter(QObject *o, QEvent *e) {
  if (o == parent() && e->type() == QEvent::Hide) {
    reject();
  }
  return QDialog::eventFilter(o, e);
}

int QDialogBase::exec() {
  setMainWindow(this);
  return QDialog::exec();
}

InputDialog::InputDialog(const QString &title, QWidget *parent, const QString &subtitle, bool secret) : QDialogBase(parent) {
  main_layout = new QVBoxLayout(this);
  main_layout->setContentsMargins(50, 55, 50, 50);
  main_layout->setSpacing(0);

  // build header
  QHBoxLayout *header_layout = new QHBoxLayout();

  QVBoxLayout *vlayout = new QVBoxLayout;
  header_layout->addLayout(vlayout);
  label = new QLabel(title, this);
  label->setStyleSheet("font-size: 90px; font-weight: bold;");
  vlayout->addWidget(label, 1, Qt::AlignTop | Qt::AlignLeft);

  if (!subtitle.isEmpty()) {
    sublabel = new QLabel(subtitle, this);
    sublabel->setStyleSheet("font-size: 55px; font-weight: light; color: #BDBDBD;");
    vlayout->addWidget(sublabel, 1, Qt::AlignTop | Qt::AlignLeft);
  }

  QPushButton* cancel_btn = new QPushButton("Cancel");
  cancel_btn->setFixedSize(386, 125);
  cancel_btn->setStyleSheet(R"(
    font-size: 48px;
    border-radius: 10px;
    color: #E4E4E4;
    background-color: #444444;
  )");
  header_layout->addWidget(cancel_btn, 0, Qt::AlignRight);
  QObject::connect(cancel_btn, &QPushButton::clicked, this, &InputDialog::reject);
  QObject::connect(cancel_btn, &QPushButton::clicked, this, &InputDialog::cancel);

  main_layout->addLayout(header_layout);

  // text box
  main_layout->addStretch(2);

  QWidget *textbox_widget = new QWidget;
  textbox_widget->setObjectName("textbox");
  QHBoxLayout *textbox_layout = new QHBoxLayout(textbox_widget);
  textbox_layout->setContentsMargins(50, 0, 50, 0);

  textbox_widget->setStyleSheet(R"(
    #textbox {
      margin-left: 50px;
      margin-right: 50px;
      border-radius: 0;
      border-bottom: 3px solid #BDBDBD;
    }
    * {
      border: none;
      font-size: 80px;
      font-weight: light;
      background-color: transparent;
    }
  )");

  line = new QLineEdit();
  line->setStyleSheet("lineedit-password-character: 8226; lineedit-password-mask-delay: 1500;");
  textbox_layout->addWidget(line, 1);

  if (secret) {
    eye_btn = new QPushButton();
    eye_btn->setCheckable(true);
    eye_btn->setFixedSize(150, 120);
    QObject::connect(eye_btn, &QPushButton::toggled, [=](bool checked) {
      if (checked) {
        eye_btn->setIcon(QIcon(ASSET_PATH + "img_eye_closed.svg"));
        eye_btn->setIconSize(QSize(81, 54));
        line->setEchoMode(QLineEdit::Password);
      } else {
        eye_btn->setIcon(QIcon(ASSET_PATH + "img_eye_open.svg"));
        eye_btn->setIconSize(QSize(81, 44));
        line->setEchoMode(QLineEdit::Normal);
      }
    });
    eye_btn->setChecked(true);
    textbox_layout->addWidget(eye_btn);
  }

  main_layout->addWidget(textbox_widget, 0, Qt::AlignBottom);
  main_layout->addSpacing(25);

  k = new Keyboard(this);
  QObject::connect(k, &Keyboard::emitEnter, this, &InputDialog::handleEnter);
  QObject::connect(k, &Keyboard::emitBackspace, this, [=]() {
    line->backspace();
  });
  QObject::connect(k, &Keyboard::emitKey, this, [=](const QString &key) {
    line->insert(key.left(1));
  });

  main_layout->addWidget(k, 2, Qt::AlignBottom);
}

QString InputDialog::getText(const QString &prompt, QWidget *parent, const QString &subtitle,
                             bool secret, int minLength, const QString &defaultText) {
  InputDialog d = InputDialog(prompt, parent, subtitle, secret);
  d.line->setText(defaultText);
  d.setMinLength(minLength);
  const int ret = d.exec();
  return ret ? d.text() : QString();
}

QString InputDialog::text() {
  return line->text();
}

void InputDialog::show() {
  setMainWindow(this);
}

void InputDialog::handleEnter() {
  if (line->text().length() >= minLength) {
    done(QDialog::Accepted);
    emitText(line->text());
  } else {
    setMessage("Need at least "+QString::number(minLength)+" characters!", false);
  }
}

void InputDialog::setMessage(const QString &message, bool clearInputField) {
  label->setText(message);
  if (clearInputField) {
    line->setText("");
  }
}

void InputDialog::setMinLength(int length) {
  minLength = length;
}

// ConfirmationDialog

ConfirmationDialog::ConfirmationDialog(const QString &prompt_text, const QString &confirm_text, const QString &cancel_text,
                                       QWidget *parent) : QDialogBase(parent) {
  QFrame *container = new QFrame(this);
  container->setStyleSheet("QFrame { border-radius: 0; background-color: #ECECEC; }");
  QVBoxLayout *main_layout = new QVBoxLayout(container);
  main_layout->setContentsMargins(32, 120, 32, 32);

  QLabel *prompt = new QLabel(prompt_text, this);
  prompt->setWordWrap(true);
  prompt->setAlignment(Qt::AlignHCenter);
  prompt->setStyleSheet("font-size: 70px; font-weight: bold; color: black;");
  main_layout->addWidget(prompt, 1, Qt::AlignTop | Qt::AlignHCenter);

  // cancel + confirm buttons
  QHBoxLayout *btn_layout = new QHBoxLayout();
  btn_layout->setSpacing(30);
  main_layout->addLayout(btn_layout);

  if (cancel_text.length()) {
    QPushButton* cancel_btn = new QPushButton(cancel_text);
    btn_layout->addWidget(cancel_btn);
    QObject::connect(cancel_btn, &QPushButton::clicked, this, &ConfirmationDialog::reject);
  }

  if (confirm_text.length()) {
    QPushButton* confirm_btn = new QPushButton(confirm_text);
    btn_layout->addWidget(confirm_btn);
    QObject::connect(confirm_btn, &QPushButton::clicked, this, &ConfirmationDialog::accept);
  }

  QVBoxLayout *outer_layout = new QVBoxLayout(this);
  outer_layout->setContentsMargins(210, 170, 210, 170);
  outer_layout->addWidget(container);
}

bool ConfirmationDialog::alert(const QString &prompt_text, QWidget *parent) {
  ConfirmationDialog d = ConfirmationDialog(prompt_text, "Ok", "", parent);
  return d.exec();
}

bool ConfirmationDialog::confirm(const QString &prompt_text, QWidget *parent) {
  ConfirmationDialog d = ConfirmationDialog(prompt_text, "Ok", "Cancel", parent);
  return d.exec();
}


// RichTextDialog

RichTextDialog::RichTextDialog(const QString &prompt_text, const QString &btn_text,
                               QWidget *parent) : QDialogBase(parent) {
  QFrame *container = new QFrame(this);
  container->setStyleSheet("QFrame { background-color: #1B1B1B; }");
  QVBoxLayout *main_layout = new QVBoxLayout(container);
  main_layout->setContentsMargins(20, 20, 20, 20);

  QLabel *prompt = new QLabel(prompt_text, this);
  prompt->setWordWrap(true);
  prompt->setAlignment(Qt::AlignLeft);
  prompt->setTextFormat(Qt::RichText);
  prompt->setStyleSheet("font-size: 40px; font-weight: light; color: #C9C9C9; margin: 25px;");
  main_layout->addWidget(new ScrollView(prompt, this), 1, Qt::AlignTop);

  // confirm button
  QPushButton* confirm_btn = new QPushButton(btn_text);
  main_layout->addWidget(confirm_btn);
  QObject::connect(confirm_btn, &QPushButton::clicked, this, &QDialog::accept);

  QVBoxLayout *outer_layout = new QVBoxLayout(this);
  outer_layout->setContentsMargins(10, 10, 10, 10);
  outer_layout->addWidget(container);
}

bool RichTextDialog::alert(const QString &prompt_text, QWidget *parent) {
  auto d = RichTextDialog(prompt_text, "Close", parent);
  return d.exec();
}

// Update Info

UpdateInfoDialog::UpdateInfoDialog(const QString &prompt_text, const QString &confirm_text, const QString &cancel_text, const QString &detail_text,
                               QWidget *parent) : QDialogBase(parent) {
  QFrame *container = new QFrame(this);
  container->setStyleSheet("QFrame { background-color: #1B1B1B; }");
  QVBoxLayout *main_layout = new QVBoxLayout(container);
  main_layout->setContentsMargins(32, 32, 32, 32);

  QLabel *title = new QLabel("UPDATE", this);
  title->setAlignment(Qt::AlignHCenter);
  title->setStyleSheet("font-size: 70px; font-weight: bold; color: #AEFF82;");
  main_layout->addWidget(title, 0, Qt::AlignTop | Qt::AlignHCenter);

  QLabel *prompt = new QLabel(prompt_text, this);
  prompt->setAlignment(Qt::AlignLeft);
  prompt->setStyleSheet("font-size: 50px; font-weight: light; color: #FFFFFF; margin: 45px;");
  main_layout->addWidget(new ScrollView(prompt, this), 1, Qt::AlignTop);

  // Update + Cancel buttons
  QHBoxLayout *btn_layout = new QHBoxLayout();
  btn_layout->setSpacing(30);
  main_layout->addLayout(btn_layout);

  if (confirm_text.length()) {
    QPushButton* confirm_btn = new QPushButton(confirm_text);
    btn_layout->addWidget(confirm_btn);
    QObject::connect(confirm_btn, &QPushButton::clicked, this, &UpdateInfoDialog::accept);
  }

  if (cancel_text.length()) {
    QPushButton* cancel_btn = new QPushButton(cancel_text);
    btn_layout->addWidget(cancel_btn);
    QObject::connect(cancel_btn, &QPushButton::clicked, this, &UpdateInfoDialog::reject);
  }

  if (detail_text.length()) {
    QPushButton* detail_btn = new QPushButton(detail_text);
    btn_layout->addWidget(detail_btn);
    QObject::connect(detail_btn, &QPushButton::clicked, [=]() {
      std::system("git log -p -5 --color=always origin/$(git rev-parse --abbrev-ref HEAD) | /data/openpilot/selfdrive/assets/addon/aha/aha > /data/git_log.html");
      const std::string txt = util::read_file("/data/git_log.html");
      RichTextDialog::alert(QString::fromStdString(txt), this);
    });
  }

  QVBoxLayout *outer_layout = new QVBoxLayout(this);
  outer_layout->setContentsMargins(30, 30, 30, 30);
  outer_layout->addWidget(container);
}

bool UpdateInfoDialog::confirm(const QString &prompt_text, QWidget *parent) {
  auto d = UpdateInfoDialog(prompt_text, "Update", "Cancel", "Detail", parent);
  return d.exec();
}

// Git Pull Cancel

GitPullCancel::GitPullCancel(const QString &confirm_text, const QString &cancel_text,
                               QWidget *parent) : QDialogBase(parent) {
  QStringList commit_list = QString::fromStdString(params.get("GitCommits")).split("\n");
  QFrame *container = new QFrame(this);
  container->setStyleSheet("QFrame { background-color: #1B1B1B; }");
  QVBoxLayout *main_layout = new QVBoxLayout(container);
  main_layout->setContentsMargins(32, 32, 32, 32);

  QLabel *title = new QLabel("Select a commit you want to go back and then press Ok to apply", this);
  title->setStyleSheet("font-size: 50px; font-weight: bold; color: #AEFF82;");
  main_layout->addWidget(title, 0, Qt::AlignTop);

  QListWidget *listWidget = new QListWidget();
  for (QString contents : commit_list) {
    listWidget->addItem(contents);
  }
  listWidget->setStyleSheet("font-size: 45px; font-weight: light; color: #FFFFFF;");
  main_layout->addWidget(new ScrollView(listWidget, this), 1);

  // Ok + Cancel buttons
  QHBoxLayout *btn_layout = new QHBoxLayout();
  btn_layout->setSpacing(30);
  main_layout->addLayout(btn_layout);

  if (confirm_text.length()) {
    QPushButton* confirm_btn = new QPushButton(confirm_text);
    btn_layout->addWidget(confirm_btn);
    QObject::connect(confirm_btn, &QPushButton::clicked, [=]() {
      int num = listWidget->currentRow();
      if (num != -1) {
        QString str = listWidget->currentItem()->text();
        QStringList hash = str.split(",");
        if (ConfirmationDialog::confirm("This will run below command\ngit reset --hard " + hash[0], this)) {
          QString cmd0 = "git reset --hard " + hash[0];
          QProcess::execute("pkill -f thermald");
          QProcess::execute("rm -f /data/openpilot/prebuilt");
          QProcess::execute(cmd0);
          QProcess::execute("reboot");
        }
      } else {
        ConfirmationDialog::alert("No selection.", this);
      }
    });
  }

  if (cancel_text.length()) {
    QPushButton* cancel_btn = new QPushButton(cancel_text);
    btn_layout->addWidget(cancel_btn);
    QObject::connect(cancel_btn, &QPushButton::clicked, this, &GitPullCancel::reject);
  }

  QVBoxLayout *outer_layout = new QVBoxLayout(this);
  outer_layout->setContentsMargins(30, 30, 30, 30);
  outer_layout->addWidget(container);
}

bool GitPullCancel::confirm(QWidget *parent) {
  auto d = GitPullCancel("Ok", "Cancel", parent);
  return d.exec();
}

