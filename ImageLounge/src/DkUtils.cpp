/*******************************************************************************************************
 DkUtils.cpp
 Created on:	09.03.2010
 
 nomacs is a fast and small image viewer with the capability of synchronizing multiple instances
 
 Copyright (C) 2011-2013 Markus Diem <markus@nomacs.org>
 Copyright (C) 2011-2013 Stefan Fiel <stefan@nomacs.org>
 Copyright (C) 2011-2013 Florian Kleber <florian@nomacs.org>

 This file is part of nomacs.

 nomacs is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 nomacs is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 *******************************************************************************************************/

#include "DkUtils.h"

#ifdef Q_WS_X11
#include <sys/sysinfo.h>
#endif

int nmc::DkUtils::debugLevel = DK_MODULE;

namespace nmc {

// code based on: http://stackoverflow.com/questions/8565430/complete-these-3-methods-with-linux-and-mac-code-memory-info-platform-independe
double DkMemory::getTotalMemory() {

	double mem = -1;

#ifdef WIN32

	MEMORYSTATUSEX MemoryStatus;
	ZeroMemory(&MemoryStatus, sizeof(MEMORYSTATUSEX));
	MemoryStatus.dwLength = sizeof(MEMORYSTATUSEX);

	if (GlobalMemoryStatusEx(&MemoryStatus)) {
		mem = MemoryStatus.ullTotalPhys;
	}

#elif defined Q_WS_X11

	struct sysinfo info;

	if (!sysinfo(&info))
		mem = info.totalram;


#elif defined Q_WS_MAC
	// TODO: could somebody (with a mac please add the corresponding calls?
#endif

	// convert to MB
	if (mem > 0)
		mem /= (1024*1024);

	return mem;
}

double DkMemory::getFreeMemory() {

	double mem = -1;
	

#ifdef WIN32

	MEMORYSTATUSEX MemoryStatus;

	ZeroMemory(&MemoryStatus, sizeof(MEMORYSTATUSEX));
	MemoryStatus.dwLength = sizeof(MEMORYSTATUSEX);

	if (GlobalMemoryStatusEx(&MemoryStatus)) {
		mem = MemoryStatus.ullAvailPhys;
	}

#elif defined Q_WS_X11

	struct sysinfo info;
	
	if (!sysinfo(&info))
		mem = info.freeram;

#elif defined Q_WS_MAC

	// TODO: could somebody with a mac please add the corresponding calls?

#endif

	// convert to MB
	if (mem > 0)
		mem /= (1024*1024);

	return mem;
}

// DkUtils --------------------------------------------------------------------
bool DkUtils::compLogicQString(const QString & lhs, const QString & rhs) {

	// check if the filenames are numbers only
	// using double here lets nomacs sort correctly for files such as "1.jpg", "1.5.jpg", "2.jpg", which is nice
	// double is accurate to approximately 16 significant digits, where using long long would work to about 19, so no big drawback.
	bool lhs_isNum;
	bool rhs_isNum;
	double lhn = lhs.left(lhs.lastIndexOf(".")).toDouble(&lhs_isNum);
	double rhn = rhs.left(rhs.lastIndexOf(".")).toDouble(&rhs_isNum);
	
	// if both filenames convert to clean numbers, compare them
	if (lhs_isNum && rhs_isNum) {
		return lhn < rhn;
	}

	// if not, let clean numbers always be sorted before mixed numbers and letters
	// logic here is deliberately spelled out, the compiler will do the optimization
	if (lhs_isNum && !rhs_isNum) {
		return true;
	}
	if (!lhs_isNum && rhs_isNum) {
		return false;
	}

	// if none are numbers, fall back to string comparison - different code for windows and linux
#ifdef WIN32

	#if QT_VERSION < 0x050000
		return StrCmpLogicalW(lhs.toStdWString(), rhs.toStdWString()) < 0;
	#else
		return StrCmpLogicalW((wchar_t*)lhs.utf16(), (wchar_t*)rhs.utf16()) < 0;	// TODO: is this nice?
	#endif

#else // not WIN32

	return lhs.localeAwareCompare(rhs) < 0;

#endif

}


bool DkUtils::compDateCreated(const QFileInfo& lhf, const QFileInfo& rhf) {

	return lhf.created() < rhf.created();
}

bool DkUtils::compDateCreatedInv(const QFileInfo& lhf, const QFileInfo& rhf) {

	return !compDateCreated(lhf, rhf);
}

bool DkUtils::compDateModified(const QFileInfo& lhf, const QFileInfo& rhf) {

	return lhf.lastModified() < rhf.lastModified();
}

bool DkUtils::compDateModifiedInv(const QFileInfo& lhf, const QFileInfo& rhf) {

	return !compDateModified(lhf, rhf);
}

bool DkUtils::compFilename(const QFileInfo& lhf, const QFileInfo& rhf) {

	return compLogicQString(lhf.fileName(), rhf.fileName());
}

bool DkUtils::compFilenameInv(const QFileInfo& lhf, const QFileInfo& rhf) {

	return !compFilename(lhf, rhf);
}

bool DkUtils::compRandom(const QFileInfo& lhf, const QFileInfo& rhf) {

	return qrand() % 2;
}

void DkUtils::addLanguages(QComboBox* langCombo, QStringList& languages) {

	QDir qmDir = qApp->applicationDirPath();
	
	// find all translations
	QStringList translationDirs = DkSettings::getTranslationDirs();
	QStringList fileNames;

	for (int idx = 0; idx < translationDirs.size(); idx++) {
		fileNames += QDir(translationDirs[idx]).entryList(QStringList("nomacs_*.qm"));
	}

	langCombo->addItem("English");
	languages << "en";

	for (int i = 0; i < fileNames.size(); ++i) {
		QString locale = fileNames[i];
		locale.remove(0, locale.indexOf('_') + 1);
		locale.chop(3);

		QTranslator translator;
		DkSettings::loadTranslation(fileNames[i], translator);

		//: this should be the name of the language in which nomacs is translated to
		QString language = translator.translate("nmc::DkGlobalSettingsWidget", "English");
		if (language.isEmpty())
			continue;

		langCombo->addItem(language);
		languages << locale;
	}
	
	langCombo->setCurrentIndex(languages.indexOf(DkSettings::global.language));
	if (langCombo->currentIndex() == -1) // set index to English if language has not been found
		langCombo->setCurrentIndex(0);

}


void DkUtils::mSleep(int ms) {

#ifdef Q_OS_WIN
	Sleep(uint(ms));
#else
	struct timespec ts = { ms / 1000, (ms % 1000) * 1000 * 1000 };
	nanosleep(&ts, NULL);
#endif

}


bool DkUtils::exists(const QFileInfo& file, int waitMs) {

	QFuture<bool> future = QtConcurrent::run(&DkUtils::checkFile, file);

	for (int idx = 0; idx < waitMs; idx++) {
		if (future.isFinished())
			break;

		//qDebug() << "you are trying the new exists method... - you are modern!";

		mSleep(1);
	}

	//future.cancel();

	// assume file is not existing if it took longer than waitMs
	return (future.isFinished()) ? future : false;	
}

bool DkUtils::checkFile(const QFileInfo& file) {

	return file.exists();
}

// code from: http://stackoverflow.com/questions/5625884/conversion-of-stdwstring-to-qstring-throws-linker-error
std::wstring DkUtils::qStringToStdWString(const QString &str) {
#ifdef _MSC_VER
	return std::wstring((const wchar_t *)str.utf16());
#else
	return str.toStdWString();
#endif
}

// code from: http://stackoverflow.com/questions/5625884/conversion-of-stdwstring-to-qstring-throws-linker-error
QString DkUtils::stdWStringToQString(const std::wstring &str) {
#ifdef _MSC_VER
	return QString::fromUtf16((const ushort *)str.c_str());
#else
	return QString::fromStdWString(str);
#endif
}


}
