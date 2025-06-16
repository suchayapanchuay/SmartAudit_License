/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Product name: redemption, a FLOSS RDP proxy
   Copyright (C) Wallix 2010-2013
   Author(s): Clément Moroldo, Jonathan Poelen, David Fort

*/

#pragma once

#include "client_redemption/client_config/client_redemption_config.hpp"
#include "utils/strutils.hpp"
#include "keyboard/keylayouts.hpp"

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QWidget>

class ClientCallback;


class QtKeyLabel :  public QWidget
{
REDEMPTION_DIAGNOSTIC_PUSH()
REDEMPTION_DIAGNOSTIC_CLANG_IGNORE("-Winconsistent-missing-override")
Q_OBJECT
REDEMPTION_DIAGNOSTIC_POP()

public:

    int q_key_code;
    QLabel label;
    bool key_not_assigned;

    QtKeyLabel(QWidget * parent)
      : QWidget(parent)
      , q_key_code(0)
      , label("Press a Key", this)
      , key_not_assigned(true)
      {}

    void set_key(int q_key_code, const std::string & q_key_name) {
        this->q_key_code = q_key_code;
        this->label.clear();
        this->label.setText(q_key_name.c_str());
    }
};



class QtOptions : public QWidget
{

REDEMPTION_DIAGNOSTIC_PUSH()
REDEMPTION_DIAGNOSTIC_CLANG_IGNORE("-Winconsistent-missing-override")
Q_OBJECT
REDEMPTION_DIAGNOSTIC_POP()

public:
    enum : uint8_t {
        RDP,
        VNC
    };
    ClientRedemptionConfig * config;
    ClientCallback * controllers;

    const int            _width;
    const int            _height;

    QTabWidget         * _tabs;
    QGridLayout        * _layout;

    QWidget            * _connectionTab;
    QFormLayout        * _layoutConnection;

    QComboBox            profilComboBox;
    QLineEdit            profilLineEdit;
    QPushButton          _buttonRestorConfig;
    QPushButton          _buttonDelConfProfil;
    QCheckBox            _recordingCB;

    QComboBox            _languageComboBox;

    QLabel               _labelProfil;
    QLabel               _labelRecording;

    QLabel               _labelLanguage;

    QWidget            * _servicesTab;
    QFormLayout        * _layoutServices;

    QCheckBox            _clipboardCheckBox;
    QCheckBox            _soundBox;

    QLabel               _labelClipboard;
    QLabel               _labelSound;

    QWidget            * _keyboardTab;
    QPushButton        * _buttonDeleteKey;
    QFormLayout        * _layoutKeyboard;

    const int            _columnNumber;
    const int            _tableKeySettingMaxHeight;

    QtOptions(ClientRedemptionConfig * config, ClientCallback * controllers, QWidget * parent)
        : QWidget(parent)
        , config(config)
        , controllers(controllers)
        , _width(410)
        , _height(330)

        , _tabs(nullptr)
        , _layout(nullptr)

        , _connectionTab(nullptr)
        , _layoutConnection(nullptr)

        , profilComboBox(this)
        , profilLineEdit("", this)
        , _buttonRestorConfig("Default configuration", this)
        , _buttonDelConfProfil("Delete current Profile", this)
        , _recordingCB(this)

    	, _languageComboBox(this)
        , _labelProfil("Options Profil:", this)
        , _labelRecording("Record movie :", this)
        , _labelLanguage("Keyboard Language :", this)

        , _servicesTab(nullptr)
        , _layoutServices(nullptr)

        , _clipboardCheckBox(this)
        , _soundBox(this)

        , _labelClipboard("Shared Clipboard :", this)
        , _labelSound("Sound :",  this)

        , _keyboardTab(nullptr)
        , _buttonDeleteKey(nullptr)

        , _layoutKeyboard(nullptr)
        , _columnNumber(4)
        , _tableKeySettingMaxHeight((20*6)+11)
    {
        this->setFixedSize(this->_width, this->_height);
        ClientConfig::setClientInfo(*this->config);
        this->_layout = new QGridLayout(this);


        this->_connectionTab = new QWidget(this);
        this->_servicesTab = new QWidget(this);
        this->_keyboardTab = new QWidget(this);
        this->_tabs = new QTabWidget(this);


        // Connection config
        const QString strConnection(" General ");
        this->_layoutConnection = new QFormLayout(this->_connectionTab);

        this->profilComboBox.setLineEdit(&(this->profilLineEdit));
        for (size_t i = 0; i < this->config->userProfils.size(); i++) {
            this->profilComboBox.addItem(this->config->userProfils[i].name.c_str(), this->config->userProfils[i].id);
        }
        this->profilComboBox.setStyleSheet("combobox-popup: 0;");
        this->_layoutConnection->addRow(&(this->_labelProfil), &(this->profilComboBox));
        this->QObject::connect(&(this->profilComboBox), SIGNAL(currentIndexChanged(int)), this, SLOT(changeProfil(int)));

        this->_buttonRestorConfig.setFixedSize(160, 20);
        this->_buttonRestorConfig.setCursor(Qt::PointingHandCursor);
        this->QObject::connect(&(this->_buttonRestorConfig) , SIGNAL (pressed()) , this, SLOT (restoreConfig()));

        this->_buttonDelConfProfil.setFixedSize(160, 20);
        this->_buttonDelConfProfil.setCursor(Qt::PointingHandCursor);
        this->QObject::connect(&(this->_buttonDelConfProfil) , SIGNAL (pressed()) , this, SLOT (deleteCurrentProtile()));
        this->_layoutConnection->addRow(&(this->_buttonRestorConfig), &(this->_buttonDelConfProfil));

        this->_layoutConnection->addRow(&(this->_labelRecording), &(this->_recordingCB));

        this->_connectionTab->setLayout(this->_layoutConnection);
        this->_tabs->addTab(this->_connectionTab, strConnection);


        // Services tab
        const QString strServices("Services");
        this->_layoutServices = new QFormLayout(this->_servicesTab);

        this->_clipboardCheckBox.setCheckState(Qt::Unchecked);
        this->_layoutServices->addRow(&(this->_labelClipboard), &(this->_clipboardCheckBox));

        this->_soundBox.setCheckState(Qt::Unchecked);
        this->_layoutServices->addRow(&(this->_labelSound), &(this->_soundBox));

        this->_servicesTab->setLayout(this->_layoutServices);
        this->_tabs->addTab(this->_servicesTab, strServices);


        // Keyboard tab
        const QString strKeyboard("Keyboard");
        this->_layoutKeyboard = new QFormLayout(this->_keyboardTab);

        for (auto const& layout : keylayouts()) {
            this->_languageComboBox.addItem(
                QString::fromLatin1(layout.name.c_str(), layout.name.size()),
                underlying_cast(layout.kbdid)
            );
        }
        this->_languageComboBox.setStyleSheet("combobox-popup: 0;");
        this->_layoutKeyboard->addRow(new QLabel("", this));
        this->_layoutKeyboard->addRow(&(this->_labelLanguage), &(this->_languageComboBox));

        this->_keyboardTab->setLayout(this->_layoutKeyboard);

        this->_tabs->addTab(this->_keyboardTab, strKeyboard);

        this->_buttonDeleteKey = new QPushButton("Delete selected row", this->_keyboardTab);
        QRect rectDeleteKey(QPoint(190, 226),QSize(180, 24));
        this->_buttonDeleteKey->setToolTip(this->_buttonDeleteKey->text());
        this->_buttonDeleteKey->setGeometry(rectDeleteKey);
        this->_buttonDeleteKey->setCursor(Qt::PointingHandCursor);
        this->QObject::connect(this->_buttonDeleteKey , SIGNAL (pressed()) , this, SLOT (deletePressed()));
        //this->_buttonDeleteKey->setEnabled(false);

        this->_layout->addWidget(this->_tabs, 0, 0, 9, 4);
        this->setLayout(this->_layout);
    }


    virtual void setConfigValues() {

        // Connection tab
    	this->_recordingCB.setChecked(this->config->is_recording);

        // Keyboard tab
        int indexLanguage = this->_languageComboBox.findData(underlying_cast(this->config->info.keylayout));
        if ( indexLanguage != -1 ) {
            this->_languageComboBox.setCurrentIndex(indexLanguage);
        }
    }

    virtual void getConfigValues() {

        //  Connection tab
        this->config->is_recording = this->_recordingCB.isChecked();

        // Keyboard tab
        this->config->modVNCParamsData.keylayout = KeyLayout::KbdId(this->_languageComboBox.itemData(this->_languageComboBox.currentIndex()).toInt());
    }

public Q_SLOTS:
    void deleteCurrentProtile() {
        if (this->profilComboBox.currentIndex() != 0) {
            ClientConfig::deleteCurrentProtile(*(this->config));
            this->profilComboBox.removeItem(this->config->current_user_profil);
            this->changeProfil(0);
        }
    }

    void restoreConfig() {
        ClientConfig::setDefaultConfig(*(this->config));
        this->setConfigValues();
    }

    void changeProfil(int index) {
        this->config->current_user_profil = this->profilComboBox.itemData(index).toInt();
        ClientConfig::setClientInfo(*this->config);
        this->setConfigValues();
    }
};



class QtRDPOptions : public QtOptions
{

REDEMPTION_DIAGNOSTIC_PUSH()
REDEMPTION_DIAGNOSTIC_CLANG_IGNORE("-Winconsistent-missing-override")
Q_OBJECT
REDEMPTION_DIAGNOSTIC_POP()

    QCheckBox            _tlsBox;
    QCheckBox            _nlaBox;

    QLabel               _labelTls;
    QLabel               _labelNla;

    QWidget            * _viewTab;
    QFormLayout        * _layoutView;

    QComboBox   _bppComboBox;
    QComboBox   _resolutionComboBox;
    QCheckBox   _remoteFxCheckBox;
    QComboBox	_networkComboBox;
    QCheckBox   _spanCheckBox;
    QCheckBox   _wallpapperCheckBox;
    QCheckBox   windowdragCheckBox;
    QCheckBox   menuanimationsCheckBox;
    QCheckBox   cursorShadowCheckBox;
    QCheckBox   cursorsettingsCheckBox;
    QCheckBox   fontSmoothingCheckBox;
    QCheckBox   desktopCompositionCheckBox;

    QLabel               _labelBpp;
    QLabel               _labelResolution;
    QLabel               _labelRemoteFx;
    QLabel				 _labelNetwork;
    QLabel               _labelSpan;
    QLabel               _labelWallpaper;
    QLabel   windowdragLabel;
    QLabel   menuanimationsLabel;
    QLabel   cursorShadowLabel;
    QLabel   cursorsettingsLabel;
    QLabel   fontSmoothingLabel;
    QLabel   desktopCompositionLabel;

    QCheckBox            _shareCheckBox;
    QLineEdit            _sharePath;
    QPushButton          _buttonSharePath;
//     QCheckBox            _soundBox;
    QCheckBox            remoteappCheckBox;
    QLineEdit            remoteapp_cmd;
    QLineEdit            remoteapp_workin_dir;
    QLabel               _labelShare;
    QLabel               _labelSharePath;
//     QLabel               _labelSound;
    QLabel               remoteappLabel;
    QLabel               remoteapp_cmd_label;
    QLabel               remoteapp_workin_dir_label;

    QCheckBox            _consoleBox;
    QLabel               _labelConsole;
    QComboBox            _captureReplayCombo;
    QLabel               _labelCaptureReplay;
    QLineEdit            _captureFileEntry;
    QLabel               _labelCaptureFile;

public:
    QtRDPOptions(ClientRedemptionConfig * config, ClientCallback * controllers,  QWidget * parent)
        : QtOptions(config, controllers, parent)
        , _tlsBox(this)
        , _nlaBox(this)
        , _labelTls("TLS :", this)
        , _labelNla("NLA :", this)
        , _viewTab(nullptr)
        , _layoutView(nullptr)
        , _bppComboBox(this)
        , _resolutionComboBox(this)
    	, _remoteFxCheckBox(this)
    	, _networkComboBox(this)
        , _spanCheckBox(this)
        , _wallpapperCheckBox(this)
        , windowdragCheckBox(this)
        , menuanimationsCheckBox(this)
        , cursorShadowCheckBox(this)
        , cursorsettingsCheckBox(this)
        , fontSmoothingCheckBox(this)
        , desktopCompositionCheckBox( this)
        , _labelBpp("Color depth :", this)
        , _labelResolution("Resolution :", this)
    	, _labelRemoteFx("Enable remoteFx :", this)
    	, _labelNetwork("Network :", this)
        , _labelSpan("Span screen :", this)
        , _labelWallpaper("Enable wallaper :", this)
        , windowdragLabel("Enable windowdrag :", this)
        , menuanimationsLabel("Enable menu animations :", this)
        , cursorShadowLabel("Enable cursor shadow", this)
        , cursorsettingsLabel("Enable cursor settings :", this)
        , fontSmoothingLabel("Enable font smoothing :", this)
        , desktopCompositionLabel("Enable desktop composition :", this)
        , _shareCheckBox(this)
        , _sharePath("", this)
        , _buttonSharePath("Select a Directory", this)
//         , _soundBox(this)
        , remoteappCheckBox(this)
        , remoteapp_cmd("", this)
        , remoteapp_workin_dir("", this)
        , _labelShare("Shared Virtual Disk :", this)
        , _labelSharePath("Shared Path :", this)
//         , _labelSound("Sound :",  this)
        , remoteappLabel("Enable remote app :", this)
        , remoteapp_cmd_label("Command line :", this)
        , remoteapp_workin_dir_label("Working direction :", this)
        , _consoleBox(this)
        , _labelConsole("Console :", this)
    	, _captureReplayCombo(this)
    	, _labelCaptureReplay("replay/capture :", this)
    	, _captureFileEntry("/tmp/capture.dump", this)
    	, _labelCaptureFile("capture file", this)

    {

        // General tab
        this->_tlsBox.setCheckState(Qt::Unchecked);
        this->_layoutConnection->addRow(&(this->_labelTls), &(this->_tlsBox));
        this->_nlaBox.setCheckState(Qt::Unchecked);
        this->_layoutConnection->addRow(&(this->_labelNla), &(this->_nlaBox));
        this->_consoleBox.setCheckState(Qt::Unchecked);
        this->_layoutConnection->addRow(&(this->_labelConsole), &(this->_consoleBox));
        this->_captureReplayCombo.addItem("none", "none");
        this->_captureReplayCombo.addItem("capture", "capture");
        this->_captureReplayCombo.addItem("replay", "replay");
        this->_layoutConnection->addRow(&this->_labelCaptureReplay, &(this->_captureReplayCombo));
        //this->_captureFileEntry.setDisabled(true);
        this->_layoutConnection->addRow(&(this->_labelCaptureFile), &(this->_captureFileEntry));


        // Services tab
        this->_shareCheckBox.setCheckState(Qt::Unchecked);
        this->QObject::connect(&(this->_shareCheckBox), SIGNAL(stateChanged(int)), this, SLOT(setEnableSharePath(int)));
        this->_layoutServices->addRow(&(this->_labelShare), &(this->_shareCheckBox));
        this->_layoutServices->addRow(&(this->_labelSharePath), &(this->_sharePath));
        QRect rectPath(QPoint(190, 226),QSize(180, 24));
        this->_buttonSharePath.setGeometry(rectPath);
        this->_buttonSharePath.setCursor(Qt::PointingHandCursor);
        this->QObject::connect(&(this->_buttonSharePath) , SIGNAL (pressed()) , this, SLOT (dirPathPressed()));
        QLabel dirButtonLabel("", this);
        this->_layoutServices->addRow(&(dirButtonLabel), &(this->_buttonSharePath));

        this->_soundBox.setCheckState(Qt::Unchecked);
        this->_layoutServices->addRow(&(this->_labelSound), &(this->_soundBox));

        this->remoteappCheckBox.setCheckState(Qt::Unchecked);
        this->QObject::connect(&(this->remoteappCheckBox), SIGNAL(stateChanged(int)), this, SLOT(setEnableRemoteApp(int)));
        this->_layoutServices->addRow(&(this->remoteappLabel), &(this->remoteappCheckBox));

        this->_layoutServices->addRow(&(this->remoteapp_cmd_label), &(this->remoteapp_cmd));

        this->remoteapp_workin_dir.setEnabled(this->config->mod_state == ClientRedemptionConfig::MOD_RDP_REMOTE_APP);
        this->_layoutServices->addRow(&(this->remoteapp_workin_dir_label), &(this->remoteapp_workin_dir));


        //  View tab
        this->_viewTab = new QWidget(this);

        const QString strView("View");
        this->_layoutView = new QFormLayout(this->_viewTab);

        this->_bppComboBox.addItem("15", 15);
        this->_bppComboBox.addItem("16", 16);
        this->_bppComboBox.addItem("24", 24);
        this->_bppComboBox.addItem("32", 32);
        this->_bppComboBox.setStyleSheet("combobox-popup: 0;");
        this->_layoutView->addRow(&(this->_labelBpp), &(this->_bppComboBox));

        this->_resolutionComboBox.addItem(" 640 * 480 ", 640);
        this->_resolutionComboBox.addItem(" 800 * 600 ", 800);
        this->_resolutionComboBox.addItem("1024 * 768 ", 1024);
        this->_resolutionComboBox.addItem("1600 * 900 ", 1600);
        this->_resolutionComboBox.addItem("1920 * 1080", 1920);
        this->_resolutionComboBox.setStyleSheet("combobox-popup: 0;");
        this->_layoutView->addRow(&(this->_labelResolution), &(this->_resolutionComboBox));

        this->_remoteFxCheckBox.setCheckState(Qt::Checked);
        this->_layoutView->addRow(&(this->_labelRemoteFx), &(this->_remoteFxCheckBox));

        this->_networkComboBox.addItem("Modem", 0x01);
        this->_networkComboBox.addItem("Broadband low", 0x02);
        this->_networkComboBox.addItem("Satellite", 0x03);
        this->_networkComboBox.addItem("Broadband high", 0x04);
        this->_networkComboBox.addItem("Wan", 0x05);
        this->_networkComboBox.addItem("LAN", 0x06);
        this->_networkComboBox.addItem("Auto detect", 0x07);
        this->_layoutView->addRow(&(this->_labelNetwork), &(this->_networkComboBox));

        this->_spanCheckBox.setCheckState(Qt::Unchecked);
        this->_layoutView->addRow(&(this->_labelSpan), &(this->_spanCheckBox));
        //this->QObject::connect(&(this->_spanCheckBox), SIGNAL(stateChanged(int)), this, SLOT(spanCheckChange(int)));

        this->_layoutView->addRow(&(this->_labelWallpaper), &(this->_wallpapperCheckBox));
        this->_layoutView->addRow(&(this->windowdragLabel), &(this->windowdragCheckBox));
        this->_layoutView->addRow(&(this->menuanimationsLabel), &(this->menuanimationsCheckBox));
        this->_layoutView->addRow(&(this->cursorShadowLabel), &(this->cursorShadowCheckBox));
        this->_layoutView->addRow(&(this->cursorsettingsLabel), &(this->cursorsettingsCheckBox));
        this->_layoutView->addRow(&(this->fontSmoothingLabel), &(this->fontSmoothingCheckBox));
        this->_layoutView->addRow(&(this->desktopCompositionLabel), &(this->desktopCompositionCheckBox));

        this->_viewTab->setLayout(this->_layoutView);
        this->_tabs->addTab(this->_viewTab, strView);

        this->setConfigValues();
    }

    void setConfigValues() override {
        QtOptions::setConfigValues();

        // General tab
        this->_tlsBox.setChecked(this->config->modRDPParamsData.enable_tls);
        this->_nlaBox.setChecked(this->config->modRDPParamsData.enable_nla);
        int indexProfil = this->profilComboBox.findData(this->config->current_user_profil);
        if ( indexProfil != -1 ) {
            this->profilComboBox.setCurrentIndex(indexProfil);
        }

        // Services tab
        this->_clipboardCheckBox.setChecked(this->config->enable_shared_clipboard);
        this->_shareCheckBox.setChecked(this->config->modRDPParamsData.enable_shared_virtual_disk);
        this->_sharePath.setEnabled(this->config->modRDPParamsData.enable_shared_virtual_disk);
        this->_sharePath.setText(this->config->SHARE_DIR.c_str());
        this->_soundBox.setChecked(this->config->modRDPParamsData.enable_sound);
        this->remoteappCheckBox.setChecked(this->config->modRDPParamsData.enable_shared_remoteapp);
        this->remoteapp_cmd.setEnabled(this->config->mod_state == ClientRedemptionConfig::MOD_RDP_REMOTE_APP);
        this->remoteapp_cmd.setText(this->config->rDPRemoteAppConfig.full_cmd_line.c_str());
        this->remoteapp_workin_dir.setEnabled(this->config->mod_state == ClientRedemptionConfig::MOD_RDP_REMOTE_APP);
        this->remoteapp_workin_dir.setText(this->config->rDPRemoteAppConfig.source_of_WorkingDir.c_str());

        // View tab
        int indexBpp = this->_bppComboBox.findData(safe_cast<int>(this->config->info.screen_info.bpp));
        if ( indexBpp != -1 ) {
            this->_bppComboBox.setCurrentIndex(indexBpp);
        }
        int indexResolution = this->_resolutionComboBox.findData(this->config->info.screen_info.width);
        if ( indexResolution != -1 ) {
            this->_resolutionComboBox.setCurrentIndex(indexResolution);
        }

        this->_remoteFxCheckBox.setChecked(this->config->enable_remotefx);
        this->_spanCheckBox.setChecked(this->config->is_spanning);
        if (this->config->info.rdp5_performanceflags & PERF_DISABLE_WALLPAPER) {
            this->_wallpapperCheckBox.setCheckState(Qt::Checked);
        }
        if (this->config->info.rdp5_performanceflags & PERF_DISABLE_FULLWINDOWDRAG) {
            this->windowdragCheckBox.setCheckState(Qt::Checked);
        }
        if (this->config->info.rdp5_performanceflags & PERF_DISABLE_MENUANIMATIONS) {
            this->menuanimationsCheckBox.setCheckState(Qt::Checked);
        }
        if (this->config->info.rdp5_performanceflags & PERF_DISABLE_CURSOR_SHADOW) {
            this->cursorShadowCheckBox.setCheckState(Qt::Checked);
        }
        if (this->config->info.rdp5_performanceflags & PERF_DISABLE_CURSORSETTINGS) {
            this->cursorsettingsCheckBox.setCheckState(Qt::Checked);
        }
        if (this->config->info.rdp5_performanceflags & PERF_ENABLE_FONT_SMOOTHING) {
            this->fontSmoothingCheckBox.setCheckState(Qt::Checked);
        }
        if (this->config->info.rdp5_performanceflags & PERF_ENABLE_DESKTOP_COMPOSITION) {
            this->desktopCompositionCheckBox.setCheckState(Qt::Checked);
        }
    }

    void getConfigValues() override {
        QtOptions::getConfigValues();

        // General tab
        bool new_profil = true;
        std::string text_profil = this->profilComboBox.currentText().toStdString();
        for (size_t i = 0; i < this->config->userProfils.size(); i++) {
            if (this->config->userProfils[i].name == text_profil) {
                new_profil = false;
            }
        }
        if (new_profil) {
            this->config->userProfils.push_back({int(this->config->userProfils.size()), text_profil.c_str()});

            this->config->current_user_profil = this->config->userProfils.size()-1;
        } else {
            this->config->current_user_profil = this->profilComboBox.currentIndex();
        }
        this->config->info.keylayout = KeyLayout::KbdId(this->_languageComboBox.itemData(this->_languageComboBox.currentIndex()).toInt());
        // this->config->update_keylayout();
        this->config->modRDPParamsData.enable_tls = this->_tlsBox.isChecked();
        this->config->modRDPParamsData.enable_nla = this->_nlaBox.isChecked();
        this->config->info.console_session = this->_consoleBox.isChecked();
        this->config->is_full_capturing = (this->_captureReplayCombo.currentText() == "capture");
        this->config->is_full_replaying = (this->_captureReplayCombo.currentText() == "replay");
        this->config->full_capture_file_name = this->_captureFileEntry.text().toStdString();


        // Services tab
        this->config->enable_shared_clipboard = this->_clipboardCheckBox.isChecked();
        if (this->_shareCheckBox.isChecked()) {
            this->config->modRDPParamsData.enable_shared_virtual_disk = true;
            this->config->SHARE_DIR = this->_sharePath.text().toStdString();
        } else {
            this->config->modRDPParamsData.enable_shared_virtual_disk = false;
        }
        this->config->modRDPParamsData.enable_sound = this->_soundBox.isChecked();
        if (this->remoteappCheckBox.isChecked()) {
            this->config->modRDPParamsData.enable_shared_remoteapp = true;
            this->config->mod_state = ClientRedemptionConfig::MOD_RDP_REMOTE_APP;

            const std::string cmd = this->remoteapp_cmd.text().toStdString();
            this->config->rDPRemoteAppConfig.full_cmd_line = cmd;
            int pos = cmd.find(' ');
            this->config->rDPRemoteAppConfig.source_of_ExeOrFile = cmd.substr(0, pos);
            this->config->rDPRemoteAppConfig.source_of_Arguments = cmd.substr(pos + 1);


            this->config->rDPRemoteAppConfig.source_of_WorkingDir = this->remoteapp_workin_dir.text().toStdString();
        } else {
            this->config->modRDPParamsData.enable_shared_remoteapp = false;
            this->config->mod_state = ClientRedemptionConfig::MOD_RDP;
        }

        //  View tab
        this->config->info.screen_info.bpp = checked_int(this->_bppComboBox.currentText().toInt());
        std::string delimiter = " * ";
        std::string resolution( this->_resolutionComboBox.currentText().toStdString());
        int pos(resolution.find(delimiter));
        this->config->info.screen_info.width  = std::stoi(resolution.substr(0, pos));
        this->config->info.screen_info.height = std::stoi(resolution.substr(pos + delimiter.length(), resolution.length()));
        this->config->enable_remotefx = this->_remoteFxCheckBox.isChecked();
        this->config->is_spanning = this->_spanCheckBox.isChecked();
        this->config->info.rdp5_performanceflags = 0;
        if (this->_wallpapperCheckBox.isChecked()) {
            this->config->info.rdp5_performanceflags |= PERF_DISABLE_WALLPAPER;
        }
        if (this->windowdragCheckBox.isChecked()) {
            this->config->info.rdp5_performanceflags |= PERF_DISABLE_FULLWINDOWDRAG;
        }
        if (this->menuanimationsCheckBox.isChecked()) {
            this->config->info.rdp5_performanceflags |= PERF_DISABLE_MENUANIMATIONS;
        }
        if (this->cursorShadowCheckBox.isChecked()) {
            this->config->info.rdp5_performanceflags |= PERF_DISABLE_CURSOR_SHADOW;
        }
        if (this->cursorsettingsCheckBox.isChecked()) {
            this->config->info.rdp5_performanceflags |= PERF_DISABLE_CURSORSETTINGS;
        }
        if (this->fontSmoothingCheckBox.isChecked()) {
            this->config->info.rdp5_performanceflags |= PERF_ENABLE_FONT_SMOOTHING;
        }
        if (this->desktopCompositionCheckBox.isChecked()) {
            this->config->info.rdp5_performanceflags |= PERF_ENABLE_DESKTOP_COMPOSITION;
        }
        this->config->info.console_session = this->_consoleBox.isChecked();
    }


public Q_SLOTS:
    void setEnableSharePath(int value) {
        this->_sharePath.setEnabled(value);
        this->_buttonSharePath.setEnabled(value);
        this->config->modRDPParamsData.enable_shared_virtual_disk = bool(value);
    }

    void setEnableRemoteApp(int value) {
        this->remoteapp_cmd.setEnabled(value);
        this->remoteapp_workin_dir.setEnabled(value);
    }

    void dirPathPressed() {
        QString filePath("");
        filePath = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                     this->config->SHARE_DIR.c_str(),
                                                     QFileDialog::ShowDirsOnly |
                                                     QFileDialog::DontResolveSymlinks);
        std::string str_share_path(filePath.toStdString());
        this->_sharePath.setText(filePath);
    }

};



class QtVNCOptions : public QtOptions
{

REDEMPTION_DIAGNOSTIC_PUSH()
REDEMPTION_DIAGNOSTIC_CLANG_IGNORE("-Winconsistent-missing-override")
Q_OBJECT
REDEMPTION_DIAGNOSTIC_POP()

    QCheckBox            keyboard_apple_compatibility_CB;
    QLabel               keyboard_apple_compatibility_label;

public:
    QtVNCOptions(ClientRedemptionConfig* config, ClientCallback * controllers, QWidget * parent)
      : QtOptions(config, controllers, parent)
        , keyboard_apple_compatibility_CB(this)
        , keyboard_apple_compatibility_label("Apple server keyboard :", this)
    {
        this->setConfigValues();

        // General tab
        this->keyboard_apple_compatibility_CB.setCheckState(Qt::Unchecked);
        this->_layoutConnection->addRow(&(this->keyboard_apple_compatibility_label), &(this->keyboard_apple_compatibility_CB));
    }

    void getConfigValues() override {
        QtOptions::getConfigValues();

        // General tab
        bool new_profil = true;
        std::string text_profil = this->profilComboBox.currentText().toStdString();
        for (size_t i = 0; i < this->config->modVNCParamsData.userProfils.size(); i++) {
            if (this->config->modVNCParamsData.userProfils[i].name == text_profil) {
                new_profil = false;
            }
        }
        if (new_profil) {
            this->config->modVNCParamsData.userProfils.push_back({int(this->config->modVNCParamsData.userProfils.size()), text_profil.c_str()});
            this->config->modVNCParamsData.current_user_profil = this->config->modVNCParamsData.userProfils.size()-1;
        } else {
            this->config->modVNCParamsData.current_user_profil = this->profilComboBox.currentIndex();
        }
        this->config->modVNCParamsData.is_apple = this->keyboard_apple_compatibility_CB.isChecked();

        // Services tab
        this->config->modVNCParamsData.enable_shared_clipboard = this->_clipboardCheckBox.isChecked();
        this->config->modVNCParamsData.enable_sound = this->_soundBox.isChecked();
    }


    void setConfigValues() override {
        QtOptions::setConfigValues();

        // General tab
        int indexProfil = this->profilComboBox.findData(this->config->modVNCParamsData.current_user_profil);
        if ( indexProfil >= 0) {
            this->profilComboBox.setCurrentIndex(indexProfil);
        }

        // Services tab
        this->_soundBox.setChecked(this->config->modVNCParamsData.enable_sound);
        this->_clipboardCheckBox.setChecked(this->config->modVNCParamsData.enable_shared_clipboard);
    }

};
