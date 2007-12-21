#include <QtCore>
#include <QtDebug>
#include <QMessageBox>
#include <plugininterface/addressparser.h>
#include <plugininterface/tcpsocket.h>
#include <plugininterface/proxy.h>
#include <exceptions/logic.h>
#include <exceptions/situative.h>
#include <exceptions/io.h>
#include "job.h"
#include "jobparams.h"
#include "jobrepresentation.h"
#include "fileexistsdialog.h"
#include "settingsmanager.h"
#include "jobmanager.h"
#include "httpimp.h"
#include "ftpimp.h"

Job::Job (JobParams *params, QObject *parent)
: QObject (parent)
, ProtoImp_ (0)
, Params_ (params)
, ErrorFlag_ (false)
, GetFileSize_ (false)
, DownloadedSize_ (0)
, TotalSize_ (0)
, RestartPosition_ (0)
, Speed_ (0)
, File_ (0)
, JobType_ (File)
{
	qRegisterMetaType<ImpBase::RemoteFileInfo> ("ImpBase::RemoteFileInfo");
	StartTime_ = new QTime;
	UpdateTime_ = new QTime;
	FlushTime_ = new QTime;
	FillErrorDictionary ();
	FileExistsDialog_ = new FileExistsDialog (parent ? qobject_cast<JobManager*> (parent)->GetTheMain () : 0);

	if (Params_->IsFullName_)
	{
		if (!QFileInfo (Params_->LocalName_).dir ().exists ())
			Params_->LocalName_ = QDir::homePath () + "/" + QFileInfo (Params_->LocalName_).fileName ();
	}
	else
		if (!QFileInfo (Params_->LocalName_).exists ())
			Params_->LocalName_ = QDir::homePath () + "/";
}

Job::~Job ()
{
	delete StartTime_;
	delete UpdateTime_;
	delete FlushTime_;
	delete ProtoImp_;
	delete File_;
	delete Params_;
}

void Job::DoDelayedInit ()
{
	QString filename = MakeFilename (Params_->URL_, Params_->LocalName_);
	if (QFile::exists (filename))
	{
		QFile *file = new QFile (filename);
		HttpImp::length_t size = file->size ();
		delete file;
		processData (size, 0, QByteArray ());
	}
	emit updateDisplays (ID_);
}

void Job::SetID (unsigned int id)
{
	ID_ = id;
}

unsigned int Job::GetID () const
{
	return ID_;
}

JobRepresentation* Job::GetRepresentation () const
{
	JobRepresentation *jr = new JobRepresentation;
	jr->ID_						= ID_;
	jr->URL_					= Params_->URL_;
	jr->LocalName_				= MakeFilename (Params_->URL_, Params_->LocalName_);
	jr->Speed_					= Speed_;
	jr->Downloaded_				= DownloadedSize_;
	jr->Size_					= TotalSize_;
	jr->ShouldBeSavedInHistory_ = Params_->ShouldBeSavedInHistory_;
	return jr;
}

bool Job::GetErrorFlag ()
{
	bool result = ErrorFlag_;
	return result;
}

QString Job::GetErrorReason ()
{
	QString result = ErrorReason_;
	return result;
}

void Job::Start ()
{
	QString ln = MakeFilename (Params_->URL_, Params_->LocalName_);
	QFileInfo fileInfo (ln);
	if (fileInfo.exists () && !fileInfo.isDir ())
		RestartPosition_ = QFile (ln).size ();

	delete File_;
	File_ = 0;
	delete ProtoImp_;

	if (Params_->URL_.left (3).toLower () == "ftp")
		ProtoImp_ = new FtpImp ();
	else
		ProtoImp_ = new HttpImp ();

	connect (ProtoImp_, SIGNAL (gotNewFiles (QStringList*)), this, SLOT (handleNewFiles (QStringList*)));
	connect (ProtoImp_, SIGNAL (clarifyURL (QString)), this, SLOT (handleClarifyURL (QString)));
	connect (ProtoImp_, SIGNAL (dataFetched (ImpBase::length_t, ImpBase::length_t, QByteArray)), this, SLOT (processData (ImpBase::length_t, ImpBase::length_t, QByteArray)), Qt::DirectConnection);
	connect (ProtoImp_, SIGNAL (finished ()), this, SLOT (reemitFinished ()), Qt::DirectConnection);
	connect (ProtoImp_, SIGNAL (error (QString)), this, SLOT (handleShowError (QString)));
	connect (ProtoImp_, SIGNAL (stopped ()), this, SLOT (reemitStopped ()));
	connect (ProtoImp_, SIGNAL (enqueue ()), this, SLOT (reemitEnqueue ()));
	connect (ProtoImp_, SIGNAL (gotRemoteFileInfo (const ImpBase::RemoteFileInfo&)), this, SLOT (handleRemoteFileInfo (const ImpBase::RemoteFileInfo&)), Qt::QueuedConnection);
	connect (ProtoImp_, SIGNAL (gotFileSize (ImpBase::length_t)), this, SLOT (reemitGotFileSize (ImpBase::length_t)));

	ProtoImp_->SetURL (Params_->URL_);
	ProtoImp_->SetRestartPosition (RestartPosition_);
	if (GetFileSize_)
		ProtoImp_->ScheduleGetFileSize ();
	GetFileSize_ = false;

	DownloadedSize_ = RestartPosition_;
	TotalSize_ = RestartPosition_;

	ProtoImp_->StartDownload ();
	StartTime_->start ();
}

void Job::GetFileSize ()
{
	GetFileSize_ = true;
	Start ();
}

void Job::handleRemoteFileInfo (const ImpBase::RemoteFileInfo& rfi)
{
	QString ln = MakeFilename (Params_->URL_, Params_->LocalName_);
	QFileInfo fileInfo (ln);
	if (fileInfo.exists () && !fileInfo.isDir ())
	{
		if (rfi.Modification_ >= fileInfo.lastModified ())
		{
			if (QMessageBox::question (parent () ? qobject_cast<JobManager*> (parent ())->GetTheMain () : 0, tr ("Question."), tr ("File on remote server is newer than local. Should I redownload it from scratch or just leave it alone?"), QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Ok)
			{
				File_ = new QFile (ln);
				if (!File_->remove ())
				{
					if (!File_->open (QFile::Truncate))
						throw Exceptions::IO (tr ("File could be neither removed, nor truncated. Check your rights or smth.").toStdString ());
					File_->close ();
				}
				File_->open (QFile::WriteOnly);
				RestartPosition_ = 0;
				Stop ();
				Start ();
				return;
			}
			else
			{
				Stop ();
				return;
			}
		}
		else
		{
			FileExistsDialog_->exec ();
			QFile f (ln);
			switch (FileExistsDialog_->GetSelected ())
			{
				case FileExistsDialog::Scratch:
					if (!QFile::remove (ln))
					{
						if (!f.open (QFile::Truncate))
							throw Exceptions::IO (tr ("File could be neither removed, nor truncated. Check your rights or smth.").toStdString ());
						f.close ();
					}
					break;
				case FileExistsDialog::Continue:
					RestartPosition_ = f.size ();
					break;
				case FileExistsDialog::Unique:
					Params_->LocalName_ = MakeUniqueNameFor (ln);
					break;
				case FileExistsDialog::Abort:
					ProtoImp_->ReactedToFileInfo ();
					Stop ();
					return;
			}
		}
	}
	File_ = new QFile (MakeFilename (Params_->URL_, Params_->LocalName_));
	ProtoImp_->ReactedToFileInfo ();
}

void Job::Stop ()
{
	if (ProtoImp_ && ProtoImp_->isRunning ())
	{
		ProtoImp_->StopDownload ();
		while (!ProtoImp_->wait (25))
			qApp->processEvents ();
	}
	reemitStopped ();
}

void Job::Release ()
{
	if (ProtoImp_ && ProtoImp_->isRunning ())
	{
		ProtoImp_->StopDownload ();
		if (!ProtoImp_->wait (SettingsManager::Instance ()->GetStopTimeout ()))
			ProtoImp_->terminate ();
	}
}

void Job::handleNewFiles (QStringList *files)
{
	QFileInfo fileInfo (Params_->LocalName_);
	QDir dir2create (QDir::root ());
	if (!fileInfo.exists () && !dir2create.mkpath (Params_->LocalName_))
	{
		emit showError (Params_->URL_, QString (tr ("Could not create directory<br /><code>%1</code><br /><br />Stopping work.")).arg (Params_->LocalName_));
		ProtoImp_->StopDownload ();
	}

	AddressParser *ap = TcpSocket::GetAddressParser (Params_->URL_);
	QString constructedURL = "ftp://" + ap->GetHost () + ":" + QString::number (ap->GetPort ());
	if (Params_->LocalName_.right (1) != "/")
		Params_->LocalName_ += "/";
	delete ap;
	for (int i = 0; i < files->size (); ++i)
	{
		JobParams *jp = new JobParams;
		if (files->at (i) [0] != '/')
			jp->URL_ = constructedURL + "/" + files->at (i);
		else
			jp->URL_ = constructedURL + files->at (i);
		jp->LocalName_ = Params_->LocalName_ + QFileInfo (files->at (i)).fileName ();
		jp->IsFullName_ = true;
		jp->Autostart_ = SettingsManager::Instance ()->GetAutostartChildren ();
		emit addJob (jp);
	}

	emit deleteJob (GetID ());
}

void Job::handleClarifyURL (QString url)
{
	delete ProtoImp_;
	ProtoImp_ = 0;

	Params_->URL_ = url;

	emit started (ID_);

	Start ();
}

void Job::processData (ImpBase::length_t ready, ImpBase::length_t total, QByteArray newData)
{
	if (newData.isEmpty () || newData.isNull ())
	{
		DownloadedSize_ = ready;
		TotalSize_ = total;
		StartTime_->restart ();
		UpdateTime_->restart ();
		FlushTime_->restart ();
		emit updateDisplays (GetID ());
		return;
	}
	else if (!File_)
		return;

	DownloadedSize_ = ready;
	TotalSize_ = total;
	Speed_ = (DownloadedSize_ - RestartPosition_) / static_cast<double> (StartTime_->elapsed ()) * 1000;

	if (UpdateTime_->elapsed () > 1000)
	{
		UpdateTime_->restart ();
		emit updateDisplays (GetID ());
	}

	if (!File_->open (QFile::WriteOnly | QFile::Append))
	{
		QTemporaryFile tmpFile ("leechcraft.httpplugin.XXXXXX");
		emit showError (Params_->URL_, QString (tr ("Could not open file for write/append<br /><code>%1</code>"
						"<br /><br />Flushing cache to temp file<br /><code>%2</code><br />and stopping work."))
						.arg (File_->fileName ())
						.arg (tmpFile.fileName ()));

		if (!tmpFile.open ())
		{
			emit showError (Params_->URL_, QString (tr ("Could not open temporary file for write<br /><code>%1</code>"))
						.arg (tmpFile.fileName ()));
		}
		else
		{
			tmpFile.write (newData);
			tmpFile.close ();
			tmpFile.setAutoRemove (false);
		}

		ProtoImp_->StopDownload ();
	}
	else
	{
		File_->write (newData);
		File_->close ();
		FlushTime_->restart ();
	}
}

void Job::reemitFinished ()
{
	qDebug () << Q_FUNC_INFO;
	emit finished (GetID ());
}

void Job::handleShowError (QString error)
{
	emit showError (Params_->URL_, error);
}

void Job::reemitStopped ()
{
	emit stopped (GetID ());
}

void Job::reemitEnqueue ()
{
	emit enqueue (GetID ());
}

void Job::reemitGotFileSize (ImpBase::length_t size)
{
	TotalSize_ = size;
	emit gotFileSize (GetID ());
}

void Job::FillErrorDictionary ()
{
	ErrorDictionary_ [QAbstractSocket::ConnectionRefusedError] = "Connection refused";
	ErrorDictionary_ [QAbstractSocket::RemoteHostClosedError] = "Remote host closed connection";
	ErrorDictionary_ [QAbstractSocket::HostNotFoundError] = "Host not found";
	ErrorDictionary_ [QAbstractSocket::SocketAccessError] = "Socket access error";
	ErrorDictionary_ [QAbstractSocket::SocketResourceError] = "Socker resource error";
	ErrorDictionary_ [QAbstractSocket::SocketTimeoutError] = "Socket timed out";
	ErrorDictionary_ [QAbstractSocket::DatagramTooLargeError] = "Datagram too large";
	ErrorDictionary_ [QAbstractSocket::NetworkError] = "Network error";
	ErrorDictionary_ [QAbstractSocket::AddressInUseError] = "Address already in use";
	ErrorDictionary_ [QAbstractSocket::SocketAddressNotAvailableError] = "Socket address not available";
	ErrorDictionary_ [QAbstractSocket::UnsupportedSocketOperationError] = "Unsupported socket operation";
	ErrorDictionary_ [QAbstractSocket::UnfinishedSocketOperationError] = "Unfinished socket operation";
	ErrorDictionary_ [QAbstractSocket::ProxyAuthenticationRequiredError] = "Proxy autentication required";
	ErrorDictionary_ [QAbstractSocket::UnknownSocketError] = "Unknown socket error";
}

QString Job::MakeUniqueNameFor (const QString& name)
{
	QString result = name;
	int i = 0;
	while (QFile (result + QString::number (i++)).exists ());
	return result + QString::number (i - 1);
}

QString Job::MakeFilename (const QString& url, QString& local) const
{
	static AddressParser *ap = TcpSocket::GetAddressParser ("");
	if (!Params_->IsFullName_)
	{
		ap->Reparse (url);
		if (!(local.at (local.size () - 1) == '/'))
			local.append ('/');
		local.append (QFileInfo (ap->GetPath ()).fileName ());
		Params_->IsFullName_ = true;
	}

	local = QUrl::fromPercentEncoding (local.toUtf8 ());

	return local;
}

