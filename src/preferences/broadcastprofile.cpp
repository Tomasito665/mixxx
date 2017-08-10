// broadcastprofile.cpp
// Created June 2nd 2017 by Stéphane Lepin <stephane.lepin@gmail.com>

#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QRegExp>
#include <QDebug>
#include <QString>
#include <QStringList>

#ifdef __QTKEYCHAIN__
#include <qtkeychain/keychain.h>
using namespace QKeychain;
#endif

#include "broadcast/defs_broadcast.h"
#include "defs_urls.h"
#include "util/xml.h"
#include "util/memory.h"

#include "broadcastprofile.h"

namespace {
const char* kDoctype = "broadcastprofile";
const char* kDocumentRoot = "BroadcastProfile";
const char* kSecureCredentials = "SecureCredentialsStorage";
const char* kBitrate = "Bitrate";
const char* kChannels = "Channels";
const char* kCustomArtist = "CustomArtist";
const char* kCustomTitle = "CustomTitle";
const char* kEnableMetadata = "EnableMetadata";
const char* kEnableReconnect = "EnableReconnect";
const char* kEnabled = "Enabled";
const char* kFormat = "Format";
const char* kHost = "Host";
const char* kLimitReconnects = "LimitReconnects";
const char* kLogin = "Login";
const char* kMaximumRetries = "MaximumRetries";
const char* kMetadataCharset = "MetadataCharset";
const char* kMetadataFormat = "MetadataFormat";
const char* kMountPoint = "Mountpoint";
const char* kNoDelayFirstReconnect = "NoDelayFirstReconnect";
const char* kOggDynamicUpdate = "OggDynamicUpdate";
const char* kPassword = "Password";
const char* kPort = "Port";
const char* kReconnectFirstDelay = "ReconnectFirstDelay";
const char* kReconnectPeriod = "ReconnectPeriod";
const char* kServertype = "Servertype";
const char* kStreamDesc = "StreamDesc";
const char* kStreamGenre = "StreamGenre";
const char* kStreamName = "StreamName";
const char* kStreamPublic = "StreamPublic";
const char* kStreamWebsite = "StreamWebsite";

const char* kKeychainPrefix = "Mixxx - ";

const double kDefaultBitrate = 128;
const int kDefaultChannels = 2;
const bool kDefaultEnableMetadata = false;
const bool kDefaultEnableReconnect = true;
const bool kDefaultLimitReconnects = true;
const int kDefaultMaximumRetries = 10;
// No tr() here, see https://bugs.launchpad.net/mixxx/+bug/1419500
const QString kDefaultMetadataFormat("$artist - $title");
const bool kDefaultNoDelayFirstReconnect = true;
const bool kDefaultOggDynamicupdate = false;
double kDefaultReconnectFirstDelay = 0.0;
double kDefaultReconnectPeriod = 5.0;
const QString kDefaultStreamDesc =
        QObject::tr("This stream is online for testing purposes!");
const QString kDefaultStreamGenre = QObject::tr("Live Mix");
const bool kDefaultStreamPublic = false;

const QRegExp kForbiddenChars =
        QRegExp("[<>:\"\\/|?*\\\\]|(\\.\\.)"
                "|CON|AUX|PRN|COM(\\d+)|LPT(\\d+)|NUL");
} // anonymous namespace

BroadcastProfile::BroadcastProfile(const QString& profileName,
                                   QObject* parent)
    : QObject(parent) {
    adoptDefaultValues();

    // Direct assignment to avoid triggering the
    // profileNameChanged signal
    m_profileName = QString(profileName);
}

bool BroadcastProfile::validName(const QString& str) {
    return !str.contains(kForbiddenChars);
}

QString BroadcastProfile::stripForbiddenChars(const QString& str) {
    QString sourceText(str);
    return sourceText.replace(kForbiddenChars, " ");
}

BroadcastProfilePtr BroadcastProfile::loadFromFile(
        const QString& filename) {
    QFileInfo xmlFile(filename);
    if(!xmlFile.exists())
        return BroadcastProfilePtr(nullptr);

    QString profileFilename = xmlFile.baseName();
    // The profile filename (without extension) is used to create the instance
    // but the real profile name (with forbidden chars but suitable for
    // non-filesystem uses) will be fetched from the XML file and set in the
    // object during the call to loadValues()
    BroadcastProfilePtr profile(new BroadcastProfile(profileFilename));
    profile->loadValues(filename);
    return profile;
}

bool BroadcastProfile::equals(BroadcastProfilePtr other) {
    return ((getProfileName() == other->getProfileName())
            && valuesEquals(other));
}

bool BroadcastProfile::valuesEquals(BroadcastProfilePtr other) {
    if(getEnabled() == other->getEnabled()
            && secureCredentialStorage() == other->secureCredentialStorage()
            && getHost() == other->getHost()
            && getPort() == other->getPort()
            && getServertype() == other->getServertype()
            && getLogin() == other->getLogin()
            && getPassword() == other->getPassword()
            && getEnableReconnect() == other->getEnableReconnect()
            && getReconnectPeriod() == other->getReconnectPeriod()
            && getLimitReconnects() == other->getLimitReconnects()
            && getMaximumRetries() == other->getMaximumRetries()
            && getNoDelayFirstReconnect() == other->getNoDelayFirstReconnect()
            && getReconnectFirstDelay() == other->getReconnectFirstDelay()
            && getFormat() == other->getFormat()
            && getBitrate() == other->getBitrate()
            && getChannels() == other->getChannels()
            && getMountpoint() == other->getMountpoint()
            && getStreamName() == other->getStreamName()
            && getStreamDesc() == other->getStreamDesc()
            && getStreamGenre() == other->getStreamGenre()
            && getStreamPublic() == other->getStreamPublic()
            && getStreamWebsite() == other->getStreamWebsite()
            && getEnableMetadata() == other->getEnableMetadata()
            && getMetadataCharset() == other->getMetadataCharset()
            && getCustomArtist() == other->getCustomArtist()
            && getCustomTitle() == other->getCustomTitle()
            && getMetadataFormat() == other->getMetadataFormat()
            && getOggDynamicUpdate() == other->getOggDynamicUpdate()) {
        return true;
    }

    return false;
}

BroadcastProfilePtr BroadcastProfile::valuesCopy() {
    BroadcastProfilePtr newProfile(
            new BroadcastProfile(getProfileName()));
    copyValuesTo(newProfile);
    return newProfile;
}

void BroadcastProfile::copyValuesTo(BroadcastProfilePtr other) {
    other->setSecureCredentialStorage(this->secureCredentialStorage());

    other->setHost(this->getHost());
    other->setPort(this->getPort());

    other->setServertype(this->getServertype());
    other->setLogin(this->getLogin());
    other->setPassword(this->getPassword());

    other->setEnableReconnect(this->getEnableReconnect());
    other->setReconnectPeriod(this->getReconnectPeriod());

    other->setLimitReconnects(this->getLimitReconnects());
    other->setMaximumRetries(this->getMaximumRetries());

    other->setNoDelayFirstReconnect(this->getNoDelayFirstReconnect());
    other->setReconnectFirstDelay(this->getReconnectFirstDelay());

    other->setFormat(this->getFormat());
    other->setBitrate(this->getBitrate());
    other->setChannels(this->getChannels());

    other->setMountPoint(this->getMountpoint());
    other->setStreamName(this->getStreamName());
    other->setStreamDesc(this->getStreamDesc());
    other->setStreamGenre(this->getStreamGenre());
    other->setStreamPublic(this->getStreamPublic());
    other->setStreamWebsite(this->getStreamWebsite());

    other->setEnableMetadata(this->getEnableMetadata());
    other->setMetadataCharset(this->getMetadataCharset());
    other->setCustomArtist(this->getCustomArtist());
    other->setCustomTitle(this->getCustomTitle());
    other->setMetadataFormat(this->getMetadataFormat());
    other->setOggDynamicUpdate(this->getOggDynamicUpdate());

    other->setEnabled(this->getEnabled());
}

void BroadcastProfile::adoptDefaultValues() {
    m_secureCredentials = false;
    m_enabled = false;

    m_host = QString();
    m_port = BROADCAST_DEFAULT_PORT;
    m_serverType = QString();
    m_login = QString();
    m_password = QString();

    m_enableReconnect = kDefaultEnableReconnect;
    m_reconnectPeriod = kDefaultReconnectPeriod;
    m_limitReconnects = kDefaultLimitReconnects;

    m_mountpoint = QString();
    m_streamDesc = kDefaultStreamDesc;
    m_streamGenre = kDefaultStreamGenre;
    m_streamName = QString();
    m_streamPublic = kDefaultStreamPublic;
    m_streamWebsite = MIXXX_WEBSITE_URL;

    m_enableMetadata = kDefaultEnableMetadata;
    m_metadataCharset = QString();
    m_customArtist = QString();
    m_customTitle = QString();
    m_metadataFormat = kDefaultMetadataFormat;
    m_oggDynamicUpdate = kDefaultOggDynamicupdate;

    m_bitrate = kDefaultBitrate;
    m_channels = kDefaultChannels;
    m_format = QString();

    m_noDelayFirstReconnect = kDefaultNoDelayFirstReconnect;
    m_reconnectFirstDelay = kDefaultReconnectFirstDelay;
    m_maximumRetries = kDefaultMaximumRetries;
}

bool BroadcastProfile::loadValues(const QString& filename) {
    QDomElement doc = XmlParse::openXMLFile(filename, kDoctype);
    if(doc.childNodes().size() < 1)
        return false;

    m_secureCredentials = (bool)XmlParse::selectNodeInt(doc, kSecureCredentials);
#ifndef __QTKEYCHAIN__
    // Secure credentials storage can't be enabled nor disabled from the UI,
    // so force it to disabled to avoid issues if enabled.
    m_secureCredentials = false;
#endif

    m_enabled = (bool)XmlParse::selectNodeInt(doc, kEnabled);

    m_host = XmlParse::selectNodeQString(doc, kHost);
    m_port = XmlParse::selectNodeInt(doc, kPort);
    m_serverType = XmlParse::selectNodeQString(doc, kServertype);

    m_login = XmlParse::selectNodeQString(doc, kLogin);
    if(m_secureCredentials) {
        m_password = getSecurePassword(m_login);
    } else {
        m_password = XmlParse::selectNodeQString(doc, kPassword);
    }

    m_enableReconnect =
            (bool)XmlParse::selectNodeInt(doc, kEnableReconnect);
    m_reconnectPeriod =
            XmlParse::selectNodeDouble(doc, kReconnectPeriod);

    m_limitReconnects =
            (bool)XmlParse::selectNodeInt(doc, kLimitReconnects);
    m_maximumRetries =
            XmlParse::selectNodeInt(doc, kMaximumRetries);

    m_noDelayFirstReconnect =
            (bool)XmlParse::selectNodeInt(doc, kNoDelayFirstReconnect);
    m_reconnectFirstDelay =
            XmlParse::selectNodeDouble(doc, kReconnectFirstDelay);

    m_mountpoint = XmlParse::selectNodeQString(doc, kMountPoint);
    m_streamName = XmlParse::selectNodeQString(doc, kStreamName);
    m_streamDesc = XmlParse::selectNodeQString(doc, kStreamDesc);
    m_streamGenre = XmlParse::selectNodeQString(doc, kStreamGenre);
    m_streamPublic = (bool)XmlParse::selectNodeInt(doc, kStreamPublic);
    m_streamWebsite = XmlParse::selectNodeQString(doc, kStreamWebsite);

    m_format = XmlParse::selectNodeQString(doc, kFormat);
    m_bitrate = XmlParse::selectNodeInt(doc, kBitrate);
    m_channels = XmlParse::selectNodeInt(doc, kChannels);

    m_enableMetadata = (bool)XmlParse::selectNodeInt(doc, kEnableMetadata);
    m_metadataCharset = XmlParse::selectNodeQString(doc, kMetadataCharset);
    m_customArtist = XmlParse::selectNodeQString(doc, kCustomArtist);
    m_customTitle = XmlParse::selectNodeQString(doc, kCustomTitle);
    m_metadataFormat = XmlParse::selectNodeQString(doc, kMetadataFormat);
    m_oggDynamicUpdate =
            (bool)XmlParse::selectNodeInt(doc, kMetadataFormat);

    return true;
}

bool BroadcastProfile::save(const QString& filename) {
    QDomDocument doc(kDoctype);
    QDomElement docRoot = doc.createElement(kDocumentRoot);

    XmlParse::addElement(doc, docRoot,
                         kSecureCredentials, QString::number((int)m_secureCredentials));
    XmlParse::addElement(doc, docRoot,
                         kEnabled, QString::number((int)m_enabled));

    XmlParse::addElement(doc, docRoot, kHost, m_host);
    XmlParse::addElement(doc, docRoot, kPort, QString::number(m_port));
    XmlParse::addElement(doc, docRoot, kServertype, m_serverType);

    XmlParse::addElement(doc, docRoot, kLogin, m_login);
    if(m_secureCredentials) {
        setSecurePassword(m_login, m_password);
    } else {
        XmlParse::addElement(doc, docRoot, kPassword, m_password);
    }

    XmlParse::addElement(doc, docRoot, kEnableReconnect,
                         QString::number((int)m_enableReconnect));
    XmlParse::addElement(doc, docRoot, kReconnectPeriod,
                         QString::number(m_reconnectPeriod));

    XmlParse::addElement(doc, docRoot, kLimitReconnects,
                         QString::number((int)m_limitReconnects));
    XmlParse::addElement(doc, docRoot, kMaximumRetries,
                         QString::number(m_maximumRetries));

    XmlParse::addElement(doc, docRoot, kNoDelayFirstReconnect,
                         QString::number((int)m_noDelayFirstReconnect));
    XmlParse::addElement(doc, docRoot, kReconnectFirstDelay,
                         QString::number(m_reconnectFirstDelay));

    XmlParse::addElement(doc, docRoot, kMountPoint, m_mountpoint);
    XmlParse::addElement(doc, docRoot, kStreamName, m_streamName);
    XmlParse::addElement(doc, docRoot, kStreamDesc, m_streamDesc);
    XmlParse::addElement(doc, docRoot, kStreamGenre, m_streamGenre);
    XmlParse::addElement(doc, docRoot, kStreamPublic,
                         QString::number((int)m_streamPublic));
    XmlParse::addElement(doc, docRoot, kStreamWebsite, m_streamWebsite);

    XmlParse::addElement(doc, docRoot, kFormat, m_format);
    XmlParse::addElement(doc, docRoot, kBitrate,
                         QString::number(m_bitrate));
    XmlParse::addElement(doc, docRoot, kChannels,
                         QString::number(m_channels));

    XmlParse::addElement(doc, docRoot, kEnableMetadata,
                         QString::number((int)m_enableMetadata));
    XmlParse::addElement(doc, docRoot, kMetadataCharset, m_metadataCharset);
    XmlParse::addElement(doc, docRoot, kCustomArtist, m_customArtist);
    XmlParse::addElement(doc, docRoot, kCustomTitle, m_customTitle);
    XmlParse::addElement(doc, docRoot, kMetadataFormat, m_metadataFormat);
    XmlParse::addElement(doc, docRoot, kOggDynamicUpdate,
                         QString::number((int)m_oggDynamicUpdate));

    doc.appendChild(docRoot);

    QFile xmlFile(filename);
    if(xmlFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream fileStream(&xmlFile);
        doc.save(fileStream, 4);
        xmlFile.close();

        return true;
    }
    return false;
}

void BroadcastProfile::setProfileName(const QString &profileName) {
    QString oldName(m_profileName);
    m_profileName = QString(profileName);

    emit profileNameChanged(oldName, m_profileName);
}

QString BroadcastProfile::getProfileName() const {
    return m_profileName;
}

void BroadcastProfile::setSecureCredentialStorage(bool value) {
    m_secureCredentials = value;
}

bool BroadcastProfile::secureCredentialStorage() {
    return m_secureCredentials;
}

bool BroadcastProfile::setSecurePassword(QString login, QString password) {
#ifdef __QTKEYCHAIN__
    QString serviceName = QString(kKeychainPrefix) + getProfileName();

    WritePasswordJob writeJob(serviceName);
    writeJob.setAutoDelete(false);
    writeJob.setKey(login);
    writeJob.setTextData(password);

    QEventLoop loop;
    writeJob.connect(&writeJob, SIGNAL(finished(QKeychain::Job*)),
                     &loop, SLOT(quit()));
    writeJob.start();
    loop.exec();

    if(writeJob.error() == Error::NoError) {
        qDebug() << "BroadcastProfile::setSecureValue: write successful";
        return true;
    } else {
        qDebug() << "BroadcastProfile::setSecureValue: write job failed with error:"
                << writeJob.errorString();
        errorDialog(tr("Can't use secure password storage: keychain access failed."),
                writeJob.errorString());
        return false;
    }
#endif
    return false;
}

QString BroadcastProfile::getSecurePassword(QString login) {
#ifdef __QTKEYCHAIN__
    QString serviceName = QString(kKeychainPrefix) + getProfileName();

    ReadPasswordJob readJob(serviceName);
    readJob.setAutoDelete(false);
    readJob.setKey(login);

    QEventLoop loop;
    readJob.connect(&readJob, SIGNAL(finished(QKeychain::Job*)),
                    &loop, SLOT(quit()));
    readJob.start();
    loop.exec();

    if(readJob.error() == Error::NoError) {
        qDebug() << "BroadcastProfile::getSecureValue: read successful";
        return readJob.textData();
    } else {
        qDebug() << "BroadcastProfile::getSecureValue: read job failed with error:"
                        << readJob.errorString();
    }
#endif
    return QString();
}

void BroadcastProfile::errorDialog(QString text, QString detailedError) {
    ErrorDialogProperties* props = ErrorDialogHandler::instance()->newDialogProperties();
    props->setType(DLG_WARNING);
    props->setTitle(tr("Live broadcasting : %1").arg(getProfileName()));
    props->setText(tr("<b>Error with settings for '%1':</b><br>")
            .arg(getProfileName()) + text);
    props->setDetails(detailedError);
    props->setKey(detailedError);   // To prevent multiple windows for the same error
    props->setDefaultButton(QMessageBox::Close);
    props->setModal(false);
    ErrorDialogHandler::instance()->requestErrorDialog(props);
}

// Used by BroadcastSettings to relay connection status
// to copies in BroadcastSettingsModel
void BroadcastProfile::relayConnectionStatus(int status) {
    setConnectionStatus(status);
}

// This was useless before, but now comes in handy for multi-broadcasting,
// where it means "this connection is enabled and will be started by Mixxx"
bool BroadcastProfile::getEnabled() const {
    return m_enabled;
}

void BroadcastProfile::setEnabled(bool value) {
    m_enabled = value;
}

QString BroadcastProfile::getHost() const {
    return m_host;
}

void BroadcastProfile::setHost(const QString& value) {
    m_host = QString(value);
}

int BroadcastProfile::getPort() const {
    // Valid port numbers are 0 .. 65535 (16 bit unsigned)
    if (m_port < 0 || m_port > 0xFFFF) {
        return BROADCAST_DEFAULT_PORT;
    }

    return m_port;
}

void BroadcastProfile::setPort(int value) {
    m_port = value;
}

QString BroadcastProfile::getServertype() const {
    return m_serverType;
}

void BroadcastProfile::setServertype(const QString& value) {
    m_serverType = QString(value);
}

QString BroadcastProfile::getLogin() const {
    return m_login;
}

void BroadcastProfile::setLogin(const QString& value) {
    m_login = QString(value);
}

// TODO(Palakis, June 2nd 2017): implement secure password storage
QString BroadcastProfile::getPassword() const {
    return m_password;
}

void BroadcastProfile::setPassword(const QString& value) {
    m_password = QString(value);
}

bool BroadcastProfile::getEnableReconnect() const {
    return m_enableReconnect;
}

void BroadcastProfile::setEnableReconnect(bool value) {
    m_enableReconnect = value;
}

double BroadcastProfile::getReconnectPeriod() const {
    return m_reconnectPeriod;
}

void BroadcastProfile::setReconnectPeriod(double value) {
    m_reconnectPeriod = value;
}

bool BroadcastProfile::getLimitReconnects() const {
    return m_limitReconnects;
}

void BroadcastProfile::setLimitReconnects(bool value) {
    m_limitReconnects = value;
}

int BroadcastProfile::getMaximumRetries() const {
    return m_maximumRetries;
}

void BroadcastProfile::setMaximumRetries(int value) {
    m_maximumRetries = value;
}

bool BroadcastProfile::getNoDelayFirstReconnect() const {
    return m_noDelayFirstReconnect;
}

void BroadcastProfile::setNoDelayFirstReconnect(bool value) {
    m_noDelayFirstReconnect = value;
}

double BroadcastProfile::getReconnectFirstDelay() const {
    return m_reconnectFirstDelay;
}

void BroadcastProfile::setReconnectFirstDelay(double value) {
    m_reconnectFirstDelay = value;
}

QString BroadcastProfile::getMountpoint() const {
    return m_mountpoint;
}

void BroadcastProfile::setMountPoint(const QString& value) {
    m_mountpoint = QString(value);
}

QString BroadcastProfile::getStreamName() const {
    return m_streamName;
}

void BroadcastProfile::setStreamName(const QString& value) {
    m_streamName = QString(value);
}

QString BroadcastProfile::getStreamDesc() const {
    return m_streamDesc;
}

void BroadcastProfile::setStreamDesc(const QString& value) {
    m_streamDesc = QString(value);
}

QString BroadcastProfile::getStreamGenre() const {
    return m_streamGenre;
}

void BroadcastProfile::setStreamGenre(const QString& value) {
    m_streamGenre = QString(value);
}

bool BroadcastProfile::getStreamPublic() const {
    return m_streamPublic;
}

void BroadcastProfile::setStreamPublic(bool value) {
    m_streamPublic = value;
}

QString BroadcastProfile::getStreamWebsite() const {
    return m_streamWebsite;
}

void BroadcastProfile::setStreamWebsite(const QString& value) {
    m_streamWebsite = QString(value);
}

QString BroadcastProfile::getFormat() const {
    return m_format;
}

void BroadcastProfile::setFormat(const QString& value) {
    m_format = QString(value);
}

int BroadcastProfile::getBitrate() const {
    return m_bitrate;
}

void BroadcastProfile::setBitrate(int value) {
    m_bitrate = value;
}

int BroadcastProfile::getChannels() const {
    return m_channels;
}

void BroadcastProfile::setChannels(int value) {
    m_channels = value;
}

bool BroadcastProfile::getEnableMetadata() const {
    return m_enableMetadata;
}

void BroadcastProfile::setEnableMetadata(bool value) {
    m_enableMetadata = value;
}

QString BroadcastProfile::getMetadataCharset() const {
    return m_metadataCharset;
}

void BroadcastProfile::setMetadataCharset(const QString& value) {
    m_metadataCharset = QString(value);
}

QString BroadcastProfile::getCustomArtist() const {
    return m_customArtist;
}

void BroadcastProfile::setCustomArtist(const QString& value) {
    m_customArtist = QString(value);
}

QString BroadcastProfile::getCustomTitle() const {
    return m_customTitle;
}

void BroadcastProfile::setCustomTitle(const QString& value) {
    m_customTitle = QString(value);
}

QString BroadcastProfile::getMetadataFormat() const {
    return m_metadataFormat;
}

void BroadcastProfile::setMetadataFormat(const QString& value) {
    m_metadataFormat = QString(value);
}

bool BroadcastProfile::getOggDynamicUpdate() const {
    return m_oggDynamicUpdate;
}

void BroadcastProfile::setOggDynamicUpdate(bool value) {
    m_oggDynamicUpdate = value;
}
