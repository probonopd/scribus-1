/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
#include "scribus13format.h"
#include "scribus13format.moc"
#include "scribus13formatimpl.h"

#include "../../formatidlist.h"
#include "commonstrings.h"
#include "missing.h"
#include "prefsmanager.h"
#include "scconfig.h"
#include "scribusdoc.h"
#include "scribusview.h"

#include "units.h"
#include "util.h"
#ifdef HAVE_LIBZ
	#include <zlib.h>
#endif
#include <qcursor.h>
#include <qfileinfo.h>
#include <qvaluelist.h>


// See scplugin.h and pluginmanager.{cpp,h} for detail on what these methods
// do. That documentatation is not duplicated here.
// Please don't implement the functionality of your plugin here; do that
// in scribus13formatimpl.h and scribus13formatimpl.cpp .

Scribus13Format::Scribus13Format() :
	LoadSavePlugin()
{
	// Set action info in languageChange, so we only have to do
	// it in one place. This includes registering file formats.
	languageChange();
}

Scribus13Format::~Scribus13Format()
{
	unregisterAll();
};

void Scribus13Format::languageChange()
{
	//(Re)register file formats.
	unregisterAll();
	registerFormats();
}

const QString Scribus13Format::fullTrName() const
{
	return QObject::tr("Scribus 1.3.0->1.3.3.2 Support");
}

const ScActionPlugin::AboutData* Scribus13Format::getAboutData() const
{
	AboutData* about = new AboutData;
	Q_CHECK_PTR(about);
	return about;
}

void Scribus13Format::deleteAboutData(const AboutData* about) const
{
	Q_ASSERT(about);
	delete about;
}

void Scribus13Format::registerFormats()
{
	FileFormat fmt(this);
	fmt.trName = tr("Scribus 1.3.x Document");
	fmt.formatId = FORMATID_SLA13XIMPORT;
	fmt.load = true;
	fmt.save = true;
#ifdef HAVE_LIBZ
	fmt.filter = fmt.trName + " (*.sla *.SLA *.sla.gz *.SLA.GZ *.scd *.SCD *.scd.gz *.SCD.GZ)";
	fmt.nameMatch = QRegExp("\\.(sla|scd)(\\.gz)?", false);
#else
	fmt.filter = fmt.trName + " (*.sla *.SLA *.scd *.SCD)";
	fmt.nameMatch = QRegExp("\\.(sla|scd)", false);
#endif
	fmt.mimeTypes = QStringList();
	fmt.mimeTypes.append("application/x-scribus");
	fmt.priority = 64;
	registerFormat(fmt);
/*	FileFormat fmt2(this);
	fmt2.trName = tr("Scribus 1.2.x Document");
	fmt2.formatId = 0;
	fmt2.load = true;
	fmt2.save = false;
	fmt2.filter = fmt.filter;
	fmt2.nameMatch = fmt.nameMatch;
	fmt2.mimeTypes.append("application/x-scribus");
	fmt2.priority = 63;
	registerFormat(fmt2);*/
}

bool Scribus13Format::fileSupported(QIODevice* /* file */, const QString & fileName) const
{
	QCString docBytes("");
	if(fileName.right(2) == "gz")
	{
#ifdef HAVE_LIBZ
		static const int gzipExpansionFactor=8;
		// The file is gzip encoded and we can load gzip files.
		// Set up to read the gzip file
		gzFile gzDoc;
		int i;
		gzDoc = gzopen(fileName.latin1(),"rb");
		if(gzDoc == NULL)
		{
			// FIXME: Needs better error return
			return "";
		}
		// Allocate a buffer of a multiple of the compressed size of the file
		// as a starting point for loading. We'll expand this buffer by powers
		// of two if we run out of space.
		const QFileInfo fi(fileName);
		uint bufSize = QMIN(4096, fi.size()*gzipExpansionFactor);
		docBytes = QCString(bufSize);
		char* buf = docBytes.data();
		uint bytesRead = 0;
		// While there's free space, read into the buffer....
		while (bytesRead<4096 && (i = gzread(gzDoc,buf,bufSize-bytesRead-1)) > 0)
		{
			// Ensure the string is null-terminated and move the
			// write pointer to the current position.
			buf[i]=0;
			buf+=i;
			bytesRead += i;
			// And check that there's free space to work with, expanding the
			// buffer if there's not.
			if (bufSize - bytesRead < 4096)
			{
				bufSize *= 2;
				docBytes.resize(bufSize);
				buf = docBytes.data() + bytesRead;
			}
		}
		gzclose(gzDoc);
#else
		// The file is gzip encoded but we can't load gzip files.
		// Leave `f' empty, since we have no way to
		// report a failure condition from here.
		return false;
#endif
	}
	else
	{
		// Not gzip encoded, just load it
		loadRawText(fileName, docBytes);
	}
	if (docBytes.left(16) == "<SCRIBUSUTF8NEW ")
		return true;
	return false;
}

QString Scribus13Format::readSLA(const QString & fileName)
{
	QCString docBytes("");
	if(fileName.right(2) == "gz")
	{
#ifdef HAVE_LIBZ
		static const int gzipExpansionFactor=8;
		// The file is gzip encoded and we can load gzip files.
		// Set up to read the gzip file
		gzFile gzDoc;
		int i;
		gzDoc = gzopen(fileName.latin1(),"rb");
		if(gzDoc == NULL)
		{
			// FIXME: Needs better error return
			return "";
		}
		// Allocate a buffer of a multiple of the compressed size of the file
		// as a starting point for loading. We'll expand this buffer by powers
		// of two if we run out of space.
		const QFileInfo fi(fileName);
		uint bufSize = fi.size()*gzipExpansionFactor;
		docBytes = QCString(bufSize);
		char* buf = docBytes.data();
		uint bytesRead = 0;
		// While there's free space, read into the buffer....
		while ((i = gzread(gzDoc,buf,bufSize-bytesRead-1)) > 0)
		{
			// Ensure the string is null-terminated and move the
			// write pointer to the current position.
			buf[i]=0;
			buf+=i;
			bytesRead += i;
			// And check that there's free space to work with, expanding the
			// buffer if there's not.
			if (bufSize - bytesRead < 4096)
			{
				bufSize *= 2;
				docBytes.resize(bufSize);
				buf = docBytes.data() + bytesRead;
			}
		}
		gzclose(gzDoc);
#else
		// The file is gzip encoded but we can't load gzip files.
		// Leave `f' empty, since we have no way to
		// report a failure condition from here.
		return false;
#endif
	}
	else
	{
		// Not gzip encoded, just load it
		loadRawText(fileName, docBytes);
	}
	QString docText("");
	if (docBytes.left(16) == "<SCRIBUSUTF8NEW ")
		docText = QString::fromUtf8(docBytes);
	else
		return QString::null;
	if (docText.endsWith(QChar(10)) || docText.endsWith(QChar(13)))
		docText.truncate(docText.length()-1);
	return docText;
}

void Scribus13Format::getReplacedFontData(bool & getNewReplacement, QMap<QString,QString> &getReplacedFonts, QPtrList<Foi> &getDummyFois)
{
	getNewReplacement=newReplacement;
	getReplacedFonts=ReplacedFonts;
	getDummyFois=dummyFois;
}

bool Scribus13Format::loadFile(const QString & fileName, const FileFormat & /* fmt */, int /* flags */, int /* index */)
{
	if (m_Doc==0 || m_AvailableFonts==0 || m_mwProgressBar==0)
	{
		Q_ASSERT(m_Doc==0 || m_AvailableFonts==0 || m_mwProgressBar==0);
		return false;
	}
	ReplacedFonts.clear();
	newReplacement = false;
	dummyFois.clear();
	ParagraphStyle vg;
	struct Layer la;
	struct ScribusDoc::BookMa bok;
	int counter;//, Pgc;
	//bool AtFl;
	bool newVersion = false;
	QString tmp, tmpf, tmp2, tmp3, tmp4, PgNam, Defont, tmf;
	QFont fo;
	QMap<int,int> TableID;
	QPtrList<PageItem> TableItems;
	int a;
	PageItem *Neu;
	Page* Apage;
	LFrames.clear();
	QDomDocument docu("scridoc");
	QString f(readSLA(fileName));
	if (f.isEmpty())
		return false;
	/* 2004/10/02 - petr vanek - bug #1092 - missing <PAGE> crash Scribus. The check constraint moved into IsScribus()
	FIXME: I've add test on containig tag PAGE but returning false freezes S. in scribus.cpp need some hack too...  */
	if (!docu.setContent(f))
		return false;
	m_Doc->PageColors.clear();
	m_Doc->Layers.clear();
	int layerToSetActive=0;
	ScColor lf = ScColor();
	QDomElement elem=docu.documentElement();
	if (elem.tagName() != "SCRIBUSUTF8NEW")
		return false;
	if (elem.hasAttribute("Version"))
		newVersion = true;
	QDomNode DOC=elem.firstChild();
	m_mwProgressBar->setTotalSteps(DOC.childNodes().count());
	m_mwProgressBar->setProgress(0);
	int ObCount = 0;
	TableItems.clear();
	TableID.clear();
	PrefsManager* prefsManager=PrefsManager::instance();
	while(!DOC.isNull())
	{
		QDomElement dc=DOC.toElement();
	/*
	* Attribute von DOCUMENT auslesen
	*/
		//CB Add this in to set this in the file in memory. Its saved, why not load it.
		//Will of course be replaced by per page settings although we still probably need a document default
		m_Doc->PageSize = dc.attribute("PAGESIZE");
		m_Doc->PageOri = dc.attribute("ORIENTATION", "0").toInt();
		m_Doc->FirstPnum = dc.attribute("FIRSTNUM", "1").toInt();
		m_Doc->currentPageLayout=dc.attribute("BOOK", "0").toInt();
		int fp;
		if (m_Doc->currentPageLayout == 0)
			fp = 0;
		else
		{
			if (dc.attribute("FIRSTLEFT", "0").toInt() == 1)
				fp = 0;
			else
				fp = 1;
		}
		if (DOC.namedItem("PageSets").isNull())
		{
			m_Doc->pageSets[m_Doc->currentPageLayout].FirstPage = fp;
			m_Doc->pageSets[m_Doc->currentPageLayout].GapHorizontal = dc.attribute("GapHorizontal", "0").toDouble();
			m_Doc->pageSets[m_Doc->currentPageLayout].GapVertical = 0.0;
			m_Doc->pageSets[m_Doc->currentPageLayout].GapBelow = dc.attribute("GapVertical", "40").toDouble();
		}
		m_Doc->setUsesAutomaticTextFrames(dc.attribute("AUTOTEXT").toInt());
		m_Doc->PageSp=dc.attribute("AUTOSPALTEN").toInt();
		m_Doc->PageSpa=dc.attribute("ABSTSPALTEN").toDouble();
		m_Doc->setUnitIndex(dc.attribute("UNITS", "0").toInt());
		m_Doc->toolSettings.defSize=qRound(dc.attribute("DSIZE").toDouble() * 10);
		Defont=dc.attribute("DFONT");
		if ((!m_AvailableFonts->find(Defont)) || (!(*m_AvailableFonts)[Defont]->usable()))
		{
			ReplacedFonts.insert(Defont, prefsManager->appPrefs.toolSettings.defFont);
			Defont = prefsManager->appPrefs.toolSettings.defFont;
		}
		else
		{
//			QFont fo = avail[Defont]->Font;
//			fo.setPointSize(qRound(m_Doc->toolSettings.defSize / 10.0));
			m_Doc->AddFont(Defont, qRound(m_Doc->toolSettings.defSize / 10.0));
		}
		m_Doc->toolSettings.defFont = Defont;
		m_Doc->toolSettings.dCols=dc.attribute("DCOL", "1").toInt();
		m_Doc->toolSettings.dGap=dc.attribute("DGAP", "0.0").toDouble();
		m_Doc->documentInfo.setAuthor(dc.attribute("AUTHOR"));
		m_Doc->documentInfo.setComments(dc.attribute("COMMENTS"));
		m_Doc->documentInfo.setKeywords(dc.attribute("KEYWORDS",""));
		m_Doc->documentInfo.setTitle(dc.attribute("TITLE"));
		m_Doc->documentInfo.setPublisher(dc.attribute("PUBLISHER", ""));
		m_Doc->documentInfo.setDate(dc.attribute("DOCDATE", ""));
		m_Doc->documentInfo.setType(dc.attribute("DOCTYPE", ""));
		m_Doc->documentInfo.setFormat(dc.attribute("DOCFORMAT", ""));
		m_Doc->documentInfo.setIdent(dc.attribute("DOCIDENT", ""));
		m_Doc->documentInfo.setSource(dc.attribute("DOCSOURCE", ""));
		m_Doc->documentInfo.setLangInfo(dc.attribute("DOCLANGINFO", ""));
		m_Doc->documentInfo.setRelation(dc.attribute("DOCRELATION", ""));
		m_Doc->documentInfo.setCover(dc.attribute("DOCCOVER", ""));
		m_Doc->documentInfo.setRights(dc.attribute("DOCRIGHTS", ""));
		m_Doc->documentInfo.setContrib(dc.attribute("DOCCONTRIB", ""));
		m_Doc->typographicSettings.valueSuperScript = dc.attribute("VHOCH").toInt();
		m_Doc->typographicSettings.scalingSuperScript = dc.attribute("VHOCHSC").toInt();
		m_Doc->typographicSettings.valueSubScript = dc.attribute("VTIEF").toInt();
		m_Doc->typographicSettings.scalingSubScript = dc.attribute("VTIEFSC").toInt();
		m_Doc->typographicSettings.valueSmallCaps = dc.attribute("VKAPIT").toInt();
		m_Doc->typographicSettings.valueBaseGrid = dc.attribute("BASEGRID", "12").toDouble();
		m_Doc->typographicSettings.offsetBaseGrid = dc.attribute("BASEO", "0").toDouble();
		m_Doc->typographicSettings.autoLineSpacing = dc.attribute("AUTOL", "20").toInt();
		m_Doc->typographicSettings.valueUnderlinePos = dc.attribute("UnderlinePos", "-1").toInt();
		m_Doc->typographicSettings.valueUnderlineWidth = dc.attribute("UnderlineWidth", "-1").toInt();
		m_Doc->typographicSettings.valueStrikeThruPos = dc.attribute("StrikeThruPos", "-1").toInt();
		m_Doc->typographicSettings.valueStrikeThruWidth = dc.attribute("StrikeThruWidth", "-1").toInt();
		m_Doc->GroupCounter=dc.attribute("GROUPC", "1").toInt();
		m_Doc->HasCMS = static_cast<bool>(dc.attribute("HCMS", "0").toInt());
		m_Doc->CMSSettings.SoftProofOn = static_cast<bool>(dc.attribute("DPSo", "0").toInt());
		m_Doc->CMSSettings.SoftProofFullOn = static_cast<bool>(dc.attribute("DPSFo", "0").toInt());
		m_Doc->CMSSettings.CMSinUse = static_cast<bool>(dc.attribute("DPuse", "0").toInt());
		m_Doc->CMSSettings.GamutCheck = static_cast<bool>(dc.attribute("DPgam", "0").toInt());
		m_Doc->CMSSettings.BlackPoint = static_cast<bool>(dc.attribute("DPbla", "1").toInt());
		m_Doc->CMSSettings.DefaultMonitorProfile = dc.attribute("DPMo","");
		m_Doc->CMSSettings.DefaultPrinterProfile = dc.attribute("DPPr","");
		m_Doc->CMSSettings.DefaultImageRGBProfile = dc.attribute("DPIn","");
		m_Doc->CMSSettings.DefaultImageCMYKProfile = dc.attribute("DPInCMYK","");
		m_Doc->CMSSettings.DefaultSolidColorRGBProfile = dc.attribute("DPIn2","");
		if (dc.hasAttribute("DPIn3"))
			m_Doc->CMSSettings.DefaultSolidColorCMYKProfile = dc.attribute("DPIn3","");
		else
			m_Doc->CMSSettings.DefaultSolidColorCMYKProfile = dc.attribute("DPPr","");
		//m_Doc->CMSSettings.DefaultIntentPrinter = dc.attribute("DIPr", "0").toInt();
		//m_Doc->CMSSettings.DefaultIntentMonitor = dc.attribute("DIMo", "1").toInt();
		m_Doc->CMSSettings.DefaultIntentColors = dc.attribute("DISc", "1").toInt();
		m_Doc->CMSSettings.DefaultIntentImages = dc.attribute("DIIm", "0").toInt();
		layerToSetActive=dc.attribute("ALAYER", "0").toInt();
		m_Doc->Language = dc.attribute("LANGUAGE", "");
		m_Doc->MinWordLen = dc.attribute("MINWORDLEN", "3").toInt();
		m_Doc->HyCount = dc.attribute("HYCOUNT", "2").toInt();
		if (dc.hasAttribute("PAGEWIDTH"))
			m_Doc->pageWidth=dc.attribute("PAGEWIDTH").toDouble();
		else
			m_Doc->pageWidth=dc.attribute("PAGEWITH").toDouble();
		m_Doc->pageHeight=dc.attribute("PAGEHEIGHT").toDouble();
		m_Doc->pageMargins.Left=QMAX(0.0, dc.attribute("BORDERLEFT").toDouble());
		m_Doc->pageMargins.Right=QMAX(0.0, dc.attribute("BORDERRIGHT").toDouble());
		m_Doc->pageMargins.Top=QMAX(0.0, dc.attribute("BORDERTOP").toDouble());
		m_Doc->pageMargins.Bottom=QMAX(0.0, dc.attribute("BORDERBOTTOM").toDouble());
		m_Doc->Automatic = static_cast<bool>(dc.attribute("AUTOMATIC", "1").toInt());
		m_Doc->AutoCheck = static_cast<bool>(dc.attribute("AUTOCHECK", "0").toInt());
		m_Doc->GuideLock = static_cast<bool>(dc.attribute("GUIDELOCK", "0").toInt());
		m_Doc->guidesSettings.minorGrid = dc.attribute("MINGRID", tmp.setNum(prefsManager->appPrefs.guidesSettings.minorGrid)).toDouble();
		m_Doc->guidesSettings.majorGrid = dc.attribute("MAJGRID", tmp.setNum(prefsManager->appPrefs.guidesSettings.majorGrid)).toDouble();
		m_Doc->guidesSettings.gridShown = static_cast<bool>(dc.attribute("SHOWGRID", "0").toInt());
		m_Doc->guidesSettings.guidesShown = static_cast<bool>(dc.attribute("SHOWGUIDES", "1").toInt());
		m_Doc->guidesSettings.colBordersShown = static_cast<bool>(dc.attribute("showcolborders", "0").toInt());
		m_Doc->guidesSettings.framesShown = static_cast<bool>(dc.attribute("SHOWFRAME", "1").toInt());
		m_Doc->guidesSettings.layerMarkersShown = static_cast<bool>(dc.attribute("SHOWLAYERM", "0").toInt());
		m_Doc->guidesSettings.marginsShown = static_cast<bool>(dc.attribute("SHOWMARGIN", "1").toInt());
		m_Doc->guidesSettings.baseShown = static_cast<bool>(dc.attribute("SHOWBASE", "0").toInt());
		m_Doc->guidesSettings.showPic = static_cast<bool>(dc.attribute("SHOWPICT", "1").toInt());
		m_Doc->guidesSettings.linkShown = static_cast<bool>(dc.attribute("SHOWLINK", "0").toInt());
		m_Doc->guidesSettings.showControls = static_cast<bool>(dc.attribute("SHOWControl", "0").toInt());
		m_Doc->guidesSettings.rulerMode = static_cast<bool>(dc.attribute("rulerMode", "1").toInt());
		m_Doc->guidesSettings.rulersShown = static_cast<bool>(dc.attribute("showrulers", "1").toInt());
		m_Doc->rulerXoffset = dc.attribute("rulerXoffset", "0").toDouble();
		m_Doc->rulerYoffset =dc.attribute("rulerYoffset", "0").toDouble();
		m_Doc->SnapGuides = static_cast<bool>(dc.attribute("SnapToGuides", "0").toInt());
		m_Doc->useRaster = static_cast<bool>(dc.attribute("SnapToGrid", "0").toInt());
		m_Doc->toolSettings.polyC = dc.attribute("POLYC", "4").toInt();
		m_Doc->toolSettings.polyF = dc.attribute("POLYF", "0.5").toDouble();
		m_Doc->toolSettings.polyR = dc.attribute("POLYR", "0").toDouble();
		m_Doc->toolSettings.polyFd = dc.attribute("POLYFD", "0").toInt();
		m_Doc->toolSettings.polyS = static_cast<bool>(dc.attribute("POLYS", "0").toInt());
		m_Doc->AutoSave = static_cast<bool>(dc.attribute("AutoSave", "0").toInt());
		m_Doc->AutoSaveTime = dc.attribute("AutoSaveTime", "600000").toInt();
		m_Doc->ScratchBottom = dc.attribute("ScratchBottom", "20").toDouble();
		// FIXME A typo in early 1.3cvs (MAR 05) means we must support loading of
		// FIXME 'ScatchLeft' for a while too. This can be removed in a few months.
		if (dc.hasAttribute("ScatchLeft"))
			m_Doc->ScratchLeft = dc.attribute("ScatchLeft", "100").toDouble();
		else
			m_Doc->ScratchLeft = dc.attribute("ScratchLeft", "100").toDouble();
		m_Doc->ScratchRight = dc.attribute("ScratchRight", "100").toDouble();
		m_Doc->ScratchTop = dc.attribute("ScratchTop", "20").toDouble();
		m_Doc->toolSettings.dStartArrow = dc.attribute("StartArrow", "0").toInt();
		m_Doc->toolSettings.dEndArrow = dc.attribute("EndArrow", "0").toInt();
		m_Doc->toolSettings.scaleX = dc.attribute("PICTSCX", "1").toDouble();
		m_Doc->toolSettings.scaleY = dc.attribute("PICTSCY", "1").toDouble();
		m_Doc->toolSettings.scaleType = static_cast<bool>(dc.attribute("PSCALE", "1").toInt());
		m_Doc->toolSettings.aspectRatio = static_cast<bool>(dc.attribute("PASPECT", "0").toInt());
		m_Doc->toolSettings.lowResType = dc.attribute("HalfRes", "1").toInt();
		m_Doc->toolSettings.useEmbeddedPath = static_cast<bool>(dc.attribute("EmbeddedPath", "0").toInt());
		if (dc.hasAttribute("PEN"))
			m_Doc->toolSettings.dPen = dc.attribute("PEN");
		if (dc.hasAttribute("BRUSH"))
			m_Doc->toolSettings.dBrush = dc.attribute("BRUSH");
		if (dc.hasAttribute("PENLINE"))
			m_Doc->toolSettings.dPenLine = dc.attribute("PENLINE");
		if (dc.hasAttribute("PENTEXT"))
			m_Doc->toolSettings.dPenText = dc.attribute("PENTEXT");
		if (dc.hasAttribute("StrokeText"))
			m_Doc->toolSettings.dStrokeText = dc.attribute("StrokeText");
		m_Doc->toolSettings.dTextBackGround = dc.attribute("TextBackGround", CommonStrings::None);
		m_Doc->toolSettings.dTextLineColor = dc.attribute("TextLineColor", CommonStrings::None);
		m_Doc->toolSettings.dTextBackGroundShade = dc.attribute("TextBackGroundShade", "100").toInt();
		m_Doc->toolSettings.dTextLineShade = dc.attribute("TextLineShade", "100").toInt();
		m_Doc->toolSettings.dTextPenShade = dc.attribute("TextPenShade", "100").toInt();
		m_Doc->toolSettings.dTextStrokeShade = dc.attribute("TextStrokeShade", "100").toInt();
		m_Doc->toolSettings.dLineArt = static_cast<Qt::PenStyle>(dc.attribute("STIL").toInt());
		m_Doc->toolSettings.dLstyleLine = static_cast<Qt::PenStyle>(dc.attribute("STILLINE").toInt());
		m_Doc->toolSettings.dWidth = dc.attribute("WIDTH", "1").toDouble();
		m_Doc->toolSettings.dWidthLine = dc.attribute("WIDTHLINE", "1").toDouble();
		m_Doc->toolSettings.dShade2 = dc.attribute("PENSHADE", "100").toInt();
		m_Doc->toolSettings.dShadeLine = dc.attribute("LINESHADE", "100").toInt();
		m_Doc->toolSettings.dShade = dc.attribute("BRUSHSHADE", "100").toInt();
		m_Doc->toolSettings.magMin = dc.attribute("MAGMIN", "10").toInt();
		m_Doc->toolSettings.magMax = dc.attribute("MAGMAX", "3200").toInt();
		m_Doc->toolSettings.magStep = dc.attribute("MAGSTEP", "200").toInt();
		//CB Reset doc zoom step value to 200% instead of old values.
		if (m_Doc->toolSettings.magStep<100)
			m_Doc->toolSettings.magStep=200;
		m_Doc->toolSettings.tabFillChar = dc.attribute("TabFill","");
		m_Doc->toolSettings.dTabWidth=dc.attribute("TabWidth", "36.0").toDouble();
		if (dc.hasAttribute("CPICT"))
			m_Doc->toolSettings.dBrushPict = dc.attribute("CPICT");
		m_Doc->toolSettings.shadePict = dc.attribute("PICTSHADE", "100").toInt();
		if (dc.hasAttribute("PAGEC"))
			m_Doc->papColor = QColor(dc.attribute("PAGEC"));
		if (dc.hasAttribute("MARGC"))
			m_Doc->guidesSettings.margColor = QColor(dc.attribute("MARGC"));
		if (dc.hasAttribute("MINORC"))
			m_Doc->guidesSettings.minorColor = QColor(dc.attribute("MINORC"));
		if (dc.hasAttribute("MAJORC"))
			m_Doc->guidesSettings.majorColor = QColor(dc.attribute("MAJORC"));
		if (dc.hasAttribute("GuideC"))
			m_Doc->guidesSettings.guideColor = QColor(dc.attribute("GuideC"));
		if (dc.hasAttribute("BaseC"))
			m_Doc->guidesSettings.baseColor = QColor(dc.attribute("BaseC"));
		m_Doc->marginColored = static_cast<bool>(dc.attribute("RANDF", "0").toInt());
		m_Doc->guidesSettings.before = static_cast<bool>(dc.attribute("BACKG", "1").toInt());
		m_Doc->guidesSettings.guideRad = dc.attribute("GuideRad", "10").toDouble();
		m_Doc->guidesSettings.grabRad = dc.attribute("GRAB", "4").toInt();
		if (dc.hasAttribute("currentProfile"))
		{
			m_Doc->checkerProfiles.clear();
			m_Doc->curCheckProfile = dc.attribute("currentProfile");
		}
		m_Doc->LastAuto = 0;
		QDomNode PAGE=DOC.firstChild();
		counter = 0;
		while(!PAGE.isNull())
		{
			ObCount++;
			m_mwProgressBar->setProgress(ObCount);
			QDomElement pg=PAGE.toElement();
			if (pg.tagName()=="PageSets")
			{
				QDomNode PGS = PAGE.firstChild();
				m_Doc->pageSets.clear();
				while(!PGS.isNull())
				{
					QDomElement PgsAttr = PGS.toElement();
					if(PgsAttr.tagName() == "Set")
					{
						struct PageSet pageS;
						pageS.Name = CommonStrings::untranslatePageSetString(PgsAttr.attribute("Name"));
						pageS.FirstPage = PgsAttr.attribute("FirstPage", "0").toInt();
						pageS.Rows = PgsAttr.attribute("Rows", "1").toInt();
						pageS.Columns = PgsAttr.attribute("Columns", "1").toInt();
						pageS.GapHorizontal = PgsAttr.attribute("GapHorizontal", "0").toDouble();
						pageS.GapVertical = PgsAttr.attribute("GapVertical", "0").toDouble();
						pageS.GapBelow = PgsAttr.attribute("GapBelow", "0").toDouble();
						pageS.pageNames.clear();
						QDomNode PGSN = PGS.firstChild();
						while(!PGSN.isNull())
						{
							QDomElement PgsAttrN = PGSN.toElement();
							if(PgsAttrN.tagName() == "PageNames")
								pageS.pageNames.append(CommonStrings::untranslatePageSetLocString(PgsAttrN.attribute("Name")));
							PGSN = PGSN.nextSibling();
						}
						m_Doc->pageSets.append(pageS);
					}
					PGS = PGS.nextSibling();
				}
			}
			if (pg.tagName()=="CheckProfile")
			{
				struct checkerPrefs checkerSettings;
				checkerSettings.ignoreErrors = static_cast<bool>(pg.attribute("ignoreErrors", "0").toInt());
				checkerSettings.autoCheck = static_cast<bool>(pg.attribute("autoCheck", "1").toInt());
				checkerSettings.checkGlyphs = static_cast<bool>(pg.attribute("checkGlyphs", "1").toInt());
				checkerSettings.checkOrphans = static_cast<bool>(pg.attribute("checkOrphans", "1").toInt());
				checkerSettings.checkOverflow = static_cast<bool>(pg.attribute("checkOverflow", "1").toInt());
				checkerSettings.checkPictures = static_cast<bool>(pg.attribute("checkPictures", "1").toInt());
				checkerSettings.checkResolution = static_cast<bool>(pg.attribute("checkResolution", "1").toInt());
				checkerSettings.checkTransparency = static_cast<bool>(pg.attribute("checkTransparency", "1").toInt());
				checkerSettings.minResolution = pg.attribute("minResolution", "72").toDouble();
				checkerSettings.maxResolution = pg.attribute("maxResolution", "4800").toDouble();
				checkerSettings.checkAnnotations = static_cast<bool>(pg.attribute("checkAnnotations", "0").toInt());
				checkerSettings.checkRasterPDF = static_cast<bool>(pg.attribute("checkRasterPDF", "1").toInt());
				checkerSettings.checkForGIF = static_cast<bool>(pg.attribute("checkForGIF", "1").toInt());
				m_Doc->checkerProfiles[pg.attribute("Name")] = checkerSettings;
			}
			// 10/25/2004 pv - None is "reserved" color. cannot be defined in any file...
			if(pg.tagName()=="COLOR" && pg.attribute("NAME")!=CommonStrings::None)
			{
				if (pg.hasAttribute("CMYK"))
					lf.setNamedColor(pg.attribute("CMYK"));
				else
					lf.fromQColor(QColor(pg.attribute("RGB")));
				if (pg.hasAttribute("Spot"))
					lf.setSpotColor(static_cast<bool>(pg.attribute("Spot").toInt()));
				else
					lf.setSpotColor(false);
				if (pg.hasAttribute("Register"))
					lf.setRegistrationColor(static_cast<bool>(pg.attribute("Register").toInt()));
				else
					lf.setRegistrationColor(false);
			  m_Doc->PageColors[pg.attribute("NAME")] = lf;
			}
			if(pg.tagName()=="STYLE")
			{
				readParagraphStyle(vg, pg, *m_AvailableFonts, m_Doc);
				m_Doc->docParagraphStyles.append(vg);
			}
			if(pg.tagName()=="JAVA")
				m_Doc->JavaScripts[pg.attribute("NAME")] = pg.attribute("SCRIPT");
			if(pg.tagName()=="LAYERS")
			{
				la.LNr = pg.attribute("NUMMER").toInt();
				la.Level = pg.attribute("LEVEL").toInt();
				la.Name = pg.attribute("NAME");
				la.isViewable = pg.attribute("SICHTBAR").toInt();
				la.isPrintable = pg.attribute("DRUCKEN").toInt();
				la.isEditable = pg.attribute("EDIT", "1").toInt();
				la.flowControl = pg.attribute("FLOW", "1").toInt();
				la.transparency = pg.attribute("TRANS", "1").toDouble();
				la.blendMode = pg.attribute("BLEND", "0").toInt();
				la.outlineMode = pg.attribute("OUTL", "0").toInt();
				if (pg.hasAttribute("LAYERC"))
					la.markerColor =  QColor(pg.attribute("LAYERC","#000000"));
				else
				{
					QColor marker;
					switch (la.LNr % 7)
					{
						case 0:
							marker = Qt::black;
							break;
						case 1:
							marker = Qt::red;
							break;
						case 2:
							marker = Qt::green;
							break;
						case 3:
							marker = Qt::blue;
							break;
						case 4:
							marker = Qt::cyan;
							break;
						case 5:
							marker = Qt::magenta;
							break;
						case 6:
							marker = Qt::yellow;;
							break;
					}
					la.markerColor = marker;
				}
				m_Doc->Layers.append(la);
			}
/*			if(pg.tagName()=="Bookmark")
			{
				bok.Title = pg.attribute("Title");
				bok.Text = pg.attribute("Text");
				bok.Aktion = pg.attribute("Aktion");
				bok.ItemNr = pg.attribute("ItemNr").toInt();
				bok.Seite = pg.attribute("Seite").toInt();
				bok.Element = pg.attribute("Element").toInt();
				bok.First = pg.attribute("First").toInt();
				bok.Last = pg.attribute("Last").toInt();
				bok.Prev = pg.attribute("Prev").toInt();
				bok.Next = pg.attribute("Next").toInt();
				bok.Parent = pg.attribute("Parent").toInt();
				m_Doc->BookMarks.append(bok);
			} */
			if(pg.tagName()=="MultiLine")
			{
				multiLine ml;
				QDomNode MuLn = PAGE.firstChild();
				while(!MuLn.isNull())
				{
					QDomElement MuL = MuLn.toElement();
					struct SingleLine sl;
					sl.Color = MuL.attribute("Color");
					sl.Dash = MuL.attribute("Dash").toInt();
					sl.LineEnd = MuL.attribute("LineEnd").toInt();
					sl.LineJoin = MuL.attribute("LineJoin").toInt();
					sl.Shade = MuL.attribute("Shade").toInt();
					sl.Width = MuL.attribute("Width").toDouble();
					ml.push_back(sl);
					MuLn = MuLn.nextSibling();
				}
				m_Doc->MLineStyles.insert(pg.attribute("Name"), ml);
			}
			if(pg.tagName()=="Arrows")
			{
				struct ArrowDesc arrow;
				arrow.name = pg.attribute("Name");
				arrow.userArrow = true;
				double xa, ya;
				QString tmp = pg.attribute("Points");
				QTextStream fp(&tmp, IO_ReadOnly);
				for (uint cx = 0; cx < pg.attribute("NumPoints").toUInt(); ++cx)
				{
					fp >> xa;
					fp >> ya;
					arrow.points.addPoint(xa, ya);
				}
				m_Doc->arrowStyles.append(arrow);
			}
			if(pg.tagName()=="PDF")
			{
				m_Doc->PDF_Options.Articles = static_cast<bool>(pg.attribute("Articles").toInt());
				m_Doc->PDF_Options.Thumbnails = static_cast<bool>(pg.attribute("Thumbnails").toInt());
				m_Doc->PDF_Options.Compress = static_cast<bool>(pg.attribute("Compress").toInt());
				m_Doc->PDF_Options.CompressMethod = pg.attribute("CMethod", "0").toInt();
				m_Doc->PDF_Options.Quality = pg.attribute("Quality", "0").toInt();
				m_Doc->PDF_Options.RecalcPic = static_cast<bool>(pg.attribute("RecalcPic").toInt());
				m_Doc->PDF_Options.Bookmarks = static_cast<bool>(pg.attribute("Bookmarks").toInt());
				if (pg.hasAttribute("MirrorH"))
					m_Doc->PDF_Options.MirrorH = static_cast<bool>(pg.attribute("MirrorH").toInt());
				else
					m_Doc->PDF_Options.MirrorH = false;
				if (pg.hasAttribute("MirrorV"))
					m_Doc->PDF_Options.MirrorV = static_cast<bool>(pg.attribute("MirrorV").toInt());
				else
					m_Doc->PDF_Options.MirrorV = false;
				if (pg.hasAttribute("RotateDeg"))
					m_Doc->PDF_Options.RotateDeg = pg.attribute("RotateDeg", "0").toInt();
				else
					m_Doc->PDF_Options.RotateDeg = 0;
				if (pg.hasAttribute("Clip"))
					m_Doc->PDF_Options.doClip = static_cast<bool>(pg.attribute("Clip").toInt());
				else
					m_Doc->PDF_Options.doClip = false;
				m_Doc->PDF_Options.PresentMode = static_cast<bool>(pg.attribute("PresentMode").toInt());
				m_Doc->PDF_Options.PicRes = pg.attribute("PicRes").toInt();
				// Fixme: check input pdf version
				m_Doc->PDF_Options.Version = (PDFOptions::PDFVersion)pg.attribute("Version").toInt();
				m_Doc->PDF_Options.Resolution = pg.attribute("Resolution").toInt();
				m_Doc->PDF_Options.Binding = pg.attribute("Binding").toInt();
				m_Doc->PDF_Options.Datei = "";
				m_Doc->PDF_Options.isGrayscale = static_cast<bool>(pg.attribute("Grayscale", "0").toInt());
				m_Doc->PDF_Options.UseRGB = static_cast<bool>(pg.attribute("RGBMode", "0").toInt());
				m_Doc->PDF_Options.UseProfiles = static_cast<bool>(pg.attribute("UseProfiles", "0").toInt());
				m_Doc->PDF_Options.UseProfiles2 = static_cast<bool>(pg.attribute("UseProfiles2", "0").toInt());
				m_Doc->PDF_Options.Intent = pg.attribute("Intent", "1").toInt();
				m_Doc->PDF_Options.Intent2 = pg.attribute("Intent2", "1").toInt();
				m_Doc->PDF_Options.SolidProf = pg.attribute("SolidP", "");
				m_Doc->PDF_Options.ImageProf = pg.attribute("ImageP", "");
				m_Doc->PDF_Options.PrintProf = pg.attribute("PrintP", "");
				m_Doc->PDF_Options.Info = pg.attribute("InfoString", "");
				m_Doc->PDF_Options.BleedTop = pg.attribute("BTop", "0").toDouble();
				m_Doc->PDF_Options.BleedLeft = pg.attribute("BLeft", "0").toDouble();
				m_Doc->PDF_Options.BleedRight = pg.attribute("BRight", "0").toDouble();
				m_Doc->PDF_Options.BleedBottom = pg.attribute("BBottom", "0").toDouble();
				m_Doc->PDF_Options.EmbeddedI = static_cast<bool>(pg.attribute("ImagePr", "0").toInt());
				m_Doc->PDF_Options.PassOwner = pg.attribute("PassOwner", "");
				m_Doc->PDF_Options.PassUser = pg.attribute("PassUser", "");
				m_Doc->PDF_Options.Permissions = pg.attribute("Permissions", "-4").toInt();
				m_Doc->PDF_Options.Encrypt = static_cast<bool>(pg.attribute("Encrypt", "0").toInt());
				m_Doc->PDF_Options.useLayers = static_cast<bool>(pg.attribute("UseLayers", "0").toInt());
				m_Doc->PDF_Options.UseLPI = static_cast<bool>(pg.attribute("UseLpi", "0").toInt());
				m_Doc->PDF_Options.UseSpotColors = static_cast<bool>(pg.attribute("UseSpotColors", "1").toInt());
				m_Doc->PDF_Options.doOverprint = static_cast<bool>(pg.attribute("doOverprint", "0").toInt());
				m_Doc->PDF_Options.doMultiFile = static_cast<bool>(pg.attribute("doMultiFile", "0").toInt());
				m_Doc->PDF_Options.displayBookmarks = static_cast<bool>(pg.attribute("displayBookmarks", "0").toInt());
				m_Doc->PDF_Options.displayFullscreen = static_cast<bool>(pg.attribute("displayFullscreen", "0").toInt());
				m_Doc->PDF_Options.displayLayers = static_cast<bool>(pg.attribute("displayLayers", "0").toInt());
				m_Doc->PDF_Options.displayThumbs = static_cast<bool>(pg.attribute("displayThumbs", "0").toInt());
				m_Doc->PDF_Options.hideMenuBar = static_cast<bool>(pg.attribute("hideMenuBar", "0").toInt());
				m_Doc->PDF_Options.hideToolBar = static_cast<bool>(pg.attribute("hideToolBar", "0").toInt());
				m_Doc->PDF_Options.fitWindow = static_cast<bool>(pg.attribute("fitWindow", "0").toInt());
				m_Doc->PDF_Options.PageLayout = pg.attribute("PageLayout", "0").toInt();
				m_Doc->PDF_Options.openAction = pg.attribute("openAction", "");
				QDomNode PFO = PAGE.firstChild();
				while(!PFO.isNull())
				{
					QDomElement pdfF = PFO.toElement();
					if(pdfF.tagName() == "LPI")
					{
						struct LPIData lpo;
						lpo.Angle = pdfF.attribute("Angle").toInt();
						lpo.Frequency = pdfF.attribute("Frequency").toInt();
						lpo.SpotFunc = pdfF.attribute("SpotFunction").toInt();
						m_Doc->PDF_Options.LPISettings[pdfF.attribute("Color")] = lpo;
					}
					if(pdfF.tagName() == "Fonts")
					{
						if (!m_Doc->PDF_Options.EmbedList.contains(pdfF.attribute("Name")))
							m_Doc->PDF_Options.EmbedList.append(pdfF.attribute("Name"));
					}
					if(pdfF.tagName() == "Subset")
					{
						if (!m_Doc->PDF_Options.SubsetList.contains(pdfF.attribute("Name")))
							m_Doc->PDF_Options.SubsetList.append(pdfF.attribute("Name"));
					}
					if(pdfF.tagName() == "Effekte")
					{
						struct PDFPresentationData ef;
						ef.pageEffectDuration = pdfF.attribute("pageEffectDuration").toInt();
						ef.pageViewDuration = pdfF.attribute("pageViewDuration").toInt();
						ef.effectType = pdfF.attribute("effectType").toInt();
						ef.Dm = pdfF.attribute("Dm").toInt();
						ef.M = pdfF.attribute("M").toInt();
						ef.Di = pdfF.attribute("Di").toInt();
						m_Doc->PDF_Options.PresentVals.append(ef);
					}
					PFO = PFO.nextSibling();
				}
			}
			if(pg.tagName()=="DocItemAttributes")
			{
				QDomNode DIA = PAGE.firstChild();
				m_Doc->docItemAttributes.clear();
				while(!DIA.isNull())
				{
					QDomElement itemAttr = DIA.toElement();
					if(itemAttr.tagName() == "ItemAttribute")
					{
						ObjectAttribute objattr;
						objattr.name=itemAttr.attribute("Name");
						objattr.type=itemAttr.attribute("Type");
						objattr.value=itemAttr.attribute("Value");
						objattr.parameter=itemAttr.attribute("Parameter");
						objattr.relationship=itemAttr.attribute("Relationship");
						objattr.relationshipto=itemAttr.attribute("RelationshipTo");
						objattr.autoaddto=itemAttr.attribute("AutoAddTo");
						m_Doc->docItemAttributes.append(objattr);
					}
					DIA = DIA.nextSibling();
				}
			}
			if(pg.tagName()=="TablesOfContents")
			{
				QDomNode TOC = PAGE.firstChild();
				m_Doc->docToCSetups.clear();
				while(!TOC.isNull())
				{
					QDomElement tocElem = TOC.toElement();
					if(tocElem.tagName() == "TableOfContents")
					{
						ToCSetup tocsetup;
						tocsetup.name=tocElem.attribute("Name");
						tocsetup.itemAttrName=tocElem.attribute("ItemAttributeName");
						tocsetup.frameName=tocElem.attribute("FrameName");
						tocsetup.listNonPrintingFrames=tocElem.attribute("ListNonPrinting");
						tocsetup.textStyle=tocElem.attribute("Style");
						QString numberPlacement=tocElem.attribute("NumberPlacement");
						if (numberPlacement=="Beginning")
							tocsetup.pageLocation=Beginning;
						if (numberPlacement=="End")
							tocsetup.pageLocation=End;
						if (numberPlacement=="NotShown")
							tocsetup.pageLocation=NotShown;
						m_Doc->docToCSetups.append(tocsetup);
					}
					TOC = TOC.nextSibling();
				}
			}
			if(pg.tagName()=="Sections")
			{
				QDomNode Section = PAGE.firstChild();
				while(!Section.isNull())
				{
					QDomElement sectionElem = Section.toElement();
					if(sectionElem.tagName() == "Section")
					{
						struct DocumentSection newSection;
						newSection.number=sectionElem.attribute("Number").toInt();
						newSection.name=sectionElem.attribute("Name");
						newSection.fromindex=sectionElem.attribute("From").toInt();
						newSection.toindex=sectionElem.attribute("To").toInt();
						if (sectionElem.attribute("Type")=="Type_1_2_3")
							newSection.type=Type_1_2_3;
						if (sectionElem.attribute("Type")=="Type_i_ii_iii")
							newSection.type=Type_i_ii_iii;
						if (sectionElem.attribute("Type")=="Type_I_II_III")
							newSection.type=Type_I_II_III;
						if (sectionElem.attribute("Type")=="Type_a_b_c")
							newSection.type=Type_a_b_c;
						if (sectionElem.attribute("Type")=="Type_A_B_C")
							newSection.type=Type_A_B_C;
						newSection.sectionstartindex=sectionElem.attribute("Start").toInt();
						newSection.reversed=static_cast<bool>(sectionElem.attribute("Reversed").toInt());
						newSection.active=static_cast<bool>(sectionElem.attribute("Active").toInt());
						m_Doc->sections.insert(newSection.number, newSection);
					}
					Section = Section.nextSibling();
				}
			}
			if ((pg.tagName()=="PAGE") || (pg.tagName()=="MASTERPAGE"))
			{
				a = pg.attribute("NUM").toInt();
				PgNam = "";
				PgNam = pg.attribute("NAM", "");
				//Pgc = m_Doc->pageCount;
				//AtFl = m_Doc->usesAutomaticTextFrames();
				if (PgNam.isEmpty())
				{
					//m_Doc->pageCount = Pgc;
					//m_Doc->Pages = &m_Doc->DocPages;
					//m_Doc->setUsesAutomaticTextFrames(AtFl);
					m_Doc->setMasterPageMode(false);
				}
				else
				{
					//m_Doc->pageCount = 0;
					//m_Doc->setUsesAutomaticTextFrames(false);
					//m_Doc->Pages = &m_Doc->MasterPages;
					m_Doc->setMasterPageMode(true);
				}
				//CB: Stop calling damn GUI code in loading docs! IT doesnt *look* like
				//this makes a difference apart from being faster, of course.
				//ScMW->slotNewPage(a);
				//Apage = m_Doc->Pages.at(a);
				if (PgNam.isEmpty())
				{
					Apage = m_Doc->addPage(a);
					//m_Doc->DocPages = m_Doc->Pages;
					//++m_Doc->pageCount;
				}
				else
				{
					Apage = m_Doc->addMasterPage(a, PgNam);
					//Apage->setPageName(PgNam);
					//m_Doc->MasterNames[PgNam] = a;
					//m_Doc->MasterPages = m_Doc->Pages;
					//m_Doc->pageCount = Pgc;
				}
				//m_Doc->setUsesAutomaticTextFrames(AtFl);
				Apage->LeftPg=pg.attribute("LEFT", "0").toInt();
				QString Mus = "";
				Mus = pg.attribute("MNAM","Normal");
				if (!m_Doc->masterPageMode())
					Apage->MPageNam = Mus;
				else
					Apage->MPageNam = "";
				if (pg.hasAttribute("Size"))
					Apage->PageSize = pg.attribute("Size");
				if (pg.hasAttribute("Orientation"))
					Apage->PageOri = pg.attribute("Orientation").toInt();
				Apage->setXOffset(pg.attribute("PAGEXPOS").toDouble());
				Apage->setYOffset(pg.attribute("PAGEYPOS").toDouble());
				if (pg.hasAttribute("PAGEWIDTH"))
					Apage->setWidth(pg.attribute("PAGEWIDTH").toDouble());
				else
					Apage->setWidth(pg.attribute("PAGEWITH").toDouble());
				Apage->setHeight(pg.attribute("PAGEHEIGHT").toDouble());
				Apage->setInitialHeight(Apage->height());
				Apage->setInitialWidth(Apage->width());
				Apage->initialMargins.Top = QMAX(0.0, pg.attribute("BORDERTOP").toDouble());
				Apage->initialMargins.Bottom = QMAX(0.0, pg.attribute("BORDERBOTTOM").toDouble());
				Apage->initialMargins.Left = QMAX(0.0, pg.attribute("BORDERLEFT").toDouble());
				Apage->initialMargins.Right = QMAX(0.0, pg.attribute("BORDERRIGHT").toDouble());
				Apage->Margins.Top = Apage->initialMargins.Top;
				Apage->Margins.Bottom = Apage->initialMargins.Bottom;
				m_Doc->setMasterPageMode(false);
				//m_Doc->Pages=&m_Doc->DocPages;
				// guides reading
				tmp = "";
				GuideManagerCore::readVerticalGuides(pg.attribute("VerticalGuides"),
						Apage,
						GuideManagerCore::Standard,
						pg.hasAttribute("NumVGuides"));
				GuideManagerCore::readHorizontalGuides(pg.attribute("HorizontalGuides"),
						Apage,
						GuideManagerCore::Standard,
						pg.hasAttribute("NumHGuides"));
			}
			if ((pg.tagName()=="PAGEOBJECT") || (pg.tagName()=="MASTEROBJECT") || (pg.tagName()=="FRAMEOBJECT"))
			{
					if ((pg.tagName()=="PAGEOBJECT") || (pg.tagName()=="FRAMEOBJECT"))
					{
						//m_Doc->Items = m_Doc->DocItems;
						//m_Doc->Pages = &m_Doc->DocPages;
						m_Doc->setMasterPageMode(false);
					}
					else
					{
						//m_Doc->Items = m_Doc->MasterItems;
						//m_Doc->Pages = &m_Doc->MasterPages;
						m_Doc->setMasterPageMode(true);
					}
					if ((!pg.attribute("OnMasterPage").isEmpty()) && (pg.tagName()=="MASTEROBJECT"))
						m_Doc->setCurrentPage(m_Doc->MasterPages.at(m_Doc->MasterNames[pg.attribute("OnMasterPage")]));
					if ((pg.attribute("NEXTITEM").toInt() != -1) || (static_cast<bool>(pg.attribute("AUTOTEXT").toInt())))
					{
						if (pg.attribute("BACKITEM").toInt() == -1)
							LFrames.append(m_Doc->Items->count());
					}
					int docGc = m_Doc->GroupCounter;
					m_Doc->GroupCounter = 0;
					Neu = PasteItem(&pg, m_Doc);
					Neu->setRedrawBounding();
					if (pg.tagName()=="MASTEROBJECT")
						Neu->OwnPage = m_Doc->OnPage(Neu);
					else
						Neu->OwnPage = pg.attribute("OwnPage").toInt();
					if (pg.tagName()=="PAGEOBJECT")
						Neu->OnMasterPage = "";
					m_Doc->GroupCounter = docGc;
					tmpf = pg.attribute("IFONT", m_Doc->toolSettings.defFont);
					if ((!m_AvailableFonts->find(tmpf)) || (!(*m_AvailableFonts)[tmpf]->usable()))
					{
						if ((!prefsManager->appPrefs.GFontSub.contains(tmpf)) || (!(*m_AvailableFonts)[prefsManager->appPrefs.GFontSub[tmpf]]->usable()))
						{
							newReplacement = true;
							ReplacedFonts.insert(tmpf, prefsManager->appPrefs.toolSettings.defFont);
						}
						else
							ReplacedFonts.insert(tmpf, prefsManager->appPrefs.GFontSub[tmpf]);
					}
					else
					{
						if (!m_Doc->UsedFonts.contains(tmpf))
						{
//							QFont fo = avail[tmpf]->Font;
//							fo.setPointSize(qRound(m_Doc->toolSettings.defSize / 10.0));
							m_Doc->AddFont(tmpf, qRound(m_Doc->toolSettings.defSize / 10.0));
						}
					}
					QDomNode IT=pg.firstChild();
					LastStyles * last = new LastStyles();
					while(!IT.isNull())
					{
						QDomElement it=IT.toElement();
						if (it.tagName()=="CSTOP")
						{
							QString name = it.attribute("NAME");
							double ramp = it.attribute("RAMP", "0.0").toDouble();
							int shade = it.attribute("SHADE", "100").toInt();
							double opa = it.attribute("TRANS", "1").toDouble();
							Neu->fill_gradient.addStop(SetColor(m_Doc, name, shade), ramp, 0.5, opa, name, shade);
						}
						if (it.tagName()=="ITEXT")
							GetItemText(&it, m_Doc, Neu, last);

						//CB PageItemAttributes
						if(it.tagName()=="PageItemAttributes")
						{
							QDomNode PIA = it.firstChild();
							ObjAttrVector pageItemAttributes;
							while(!PIA.isNull())
							{
								QDomElement itemAttr = PIA.toElement();
								if(itemAttr.tagName() == "ItemAttribute")
								{
									ObjectAttribute objattr;
									objattr.name=itemAttr.attribute("Name");
									objattr.type=itemAttr.attribute("Type");
									objattr.value=itemAttr.attribute("Value");
									objattr.parameter=itemAttr.attribute("Parameter");
									objattr.relationship=itemAttr.attribute("Relationship");
									objattr.relationshipto=itemAttr.attribute("RelationshipTo");
									objattr.autoaddto=itemAttr.attribute("AutoAddTo");
									pageItemAttributes.append(objattr);
								}
								PIA = PIA.nextSibling();
							}
							Neu->setObjectAttributes(&pageItemAttributes);
						}
						IT=IT.nextSibling();
					}
					delete last;
					if (Neu->fill_gradient.Stops() == 0)
					{
						Neu->fill_gradient.addStop(m_Doc->PageColors[m_Doc->toolSettings.dBrush].getRGBColor(), 0.0, 0.5, 1.0, m_Doc->toolSettings.dBrush, 100);
						Neu->fill_gradient.addStop(m_Doc->PageColors[m_Doc->toolSettings.dPen].getRGBColor(), 1.0, 0.5, 1.0, m_Doc->toolSettings.dPen, 100);
					}
//					Neu->Language = ScMW->GetLang(pg.attribute("LANGUAGE", m_Doc->Language));
					Neu->isAutoText = static_cast<bool>(pg.attribute("AUTOTEXT").toInt());
					Neu->isEmbedded = static_cast<bool>(pg.attribute("isInline", "0").toInt());
					Neu->gXpos = pg.attribute("gXpos", "0.0").toDouble();
					Neu->gYpos = pg.attribute("gYpos", "0.0").toDouble();
					QString defaultVal;
					defaultVal.setNum(Neu->width());
					Neu->gWidth = pg.attribute("gWidth",defaultVal).toDouble();
					defaultVal.setNum(Neu->height());
					Neu->gHeight = pg.attribute("gHeight",defaultVal).toDouble();
/*FIXME					if (Neu->lineSpacingMode() == 3)
					{
						m_Doc->docParagraphStyles[0].setUseBaselineGrid(true);
						Neu->setLineSpacing(m_Doc->typographicSettings.valueBaseGrid-1);
					}
*/					if (Neu->isAutoText)
						m_Doc->LastAuto = Neu;
					Neu->NextIt = pg.attribute("NEXTITEM").toInt();
					if (Neu->isTableItem)
					{
						TableItems.append(Neu);
						TableID.insert(pg.attribute("OwnLINK", "0").toInt(), Neu->ItemNr);
					}
					if (pg.tagName()=="FRAMEOBJECT")
					{
						m_Doc->FrameItems.append(m_Doc->Items->take(Neu->ItemNr));
						Neu->ItemNr = m_Doc->FrameItems.count()-1;
					}
					/*
					if ((pg.tagName()=="PAGEOBJECT") || (pg.tagName()=="FRAMEOBJECT"))
					{
						//m_Doc->DocItems = m_Doc->Items;
						//m_Doc->DocPages = m_Doc->Pages;
					}
					else
					{
						//m_Doc->MasterItems = m_Doc->Items;
						//m_Doc->MasterPages = m_Doc->Pages;
					}
					*/
					m_Doc->setMasterPageMode(false);
					//m_Doc->Pages=&m_Doc->DocPages;
					counter++;
				}
			PAGE=PAGE.nextSibling();
		}
		PAGE=DOC.firstChild();
		while(!PAGE.isNull())
		{
			QDomElement pg=PAGE.toElement();
			if(pg.tagName()=="Bookmark")
			{
				bok.Title = pg.attribute("Title");
				bok.Text = pg.attribute("Text");
				bok.Aktion = pg.attribute("Aktion");
				bok.ItemNr = pg.attribute("ItemNr").toInt();
				bok.PageObject = m_Doc->Items->at(pg.attribute("Element").toInt());
				bok.First = pg.attribute("First").toInt();
				bok.Last = pg.attribute("Last").toInt();
				bok.Prev = pg.attribute("Prev").toInt();
				bok.Next = pg.attribute("Next").toInt();
				bok.Parent = pg.attribute("Parent").toInt();
				m_Doc->BookMarks.append(bok);
			}
			PAGE=PAGE.nextSibling();
		}
		DOC=DOC.nextSibling();
	}
	if (TableItems.count() != 0)
	{
		for (uint ttc = 0; ttc < TableItems.count(); ++ttc)
		{
			PageItem* ta = TableItems.at(ttc);
			if (ta->TopLinkID != -1)
				ta->TopLink = m_Doc->Items->at(TableID[ta->TopLinkID]);
			else
				ta->TopLink = 0;
			if (ta->LeftLinkID != -1)
				ta->LeftLink = m_Doc->Items->at(TableID[ta->LeftLinkID]);
			else
				ta->LeftLink = 0;
			if (ta->RightLinkID != -1)
				ta->RightLink = m_Doc->Items->at(TableID[ta->RightLinkID]);
			else
				ta->RightLink = 0;
			if (ta->BottomLinkID != -1)
				ta->BottomLink = m_Doc->Items->at(TableID[ta->BottomLinkID]);
			else
				ta->BottomLink = 0;
		}
	}
	m_Doc->setActiveLayer(layerToSetActive);
	m_Doc->setMasterPageMode(false);
	m_Doc->reformPages();

	if (m_Doc->Layers.count() == 0)
	{
		la.LNr = 0;
		la.Level = 0;
		la.Name = QObject::tr("Background");
		la.isViewable = true;
		la.isPrintable = true;
		la.isEditable = true;
		la.flowControl = true;
		la.transparency = 1.0;
		la.blendMode = 0;
		la.markerColor = QColor(0, 0, 0);
		la.outlineMode = false;
		m_Doc->Layers.append(la);
	}
	if (LFrames.count() != 0)
	{
		PageItem *Its;
		PageItem *Itn;
		PageItem *Itr;
		QValueList<int>::Iterator lc;
		for (lc = LFrames.begin(); lc != LFrames.end(); ++lc)
		{
			Its = m_Doc->Items->at((*lc));
			Itr = Its;
			Its->BackBox = 0;
			if (Its->isAutoText)
				m_Doc->FirstAuto = Its;
			while (Its->NextIt != -1)
			{
				Itn = m_Doc->Items->at(Its->NextIt);
				Its->NextBox = Itn;
				Itn->BackBox = Its;
				Its = Itn;
			}
			Its->NextBox = 0;
		}
	}
	m_mwProgressBar->setProgress(DOC.childNodes().count());
	return true;
// 	return false;
}

bool Scribus13Format::saveFile(const QString & /* fileName */, const FileFormat & /* fmt */)
{
	return false;
}

// Low level plugin API
int scribus13format_getPluginAPIVersion()
{
	return PLUGIN_API_VERSION;
}

ScPlugin* scribus13format_getPlugin()
{
	Scribus13Format* plug = new Scribus13Format();
	Q_CHECK_PTR(plug);
	return plug;
}

void scribus13format_freePlugin(ScPlugin* plugin)
{
	Scribus13Format* plug = dynamic_cast<Scribus13Format*>(plugin);
	Q_ASSERT(plug);
	delete plug;
}

void breakPoint() {}

void Scribus13Format::GetItemText(QDomElement *it, ScribusDoc *doc, PageItem* obj, LastStyles* last, bool impo, bool VorLFound)
{
	Foi* dummy;
	bool unknown = false;
	QString tmp2, tmpf;
	tmp2 = it->attribute("CH");
	tmp2.replace(QRegExp("\r"), QChar(13));
	tmp2.replace(QRegExp("\n"), QChar(13));
	tmp2.replace(QRegExp("\t"), QChar(9));
	tmpf = it->attribute("CFONT", doc->toolSettings.defFont);
	PrefsManager* prefsManager=PrefsManager::instance();
	if ((!prefsManager->appPrefs.AvailFonts.find(tmpf)) || (!prefsManager->appPrefs.AvailFonts[tmpf]->usable()))
	{
		bool isThere = false;
		for (uint dl = 0; dl < dummyFois.count(); ++dl)
		{
			if (dummyFois.at(dl)->scName() == tmpf)
			{
				isThere = true;
				dummy = dummyFois.at(dl);
				break;
			}
		}
		if (!isThere)
		{
			dummy = new Foi(tmpf, "", tmpf, "", "", 1, false);
			dummyFois.append(dummy);
		}
		unknown = true;
		if ((!prefsManager->appPrefs.GFontSub.contains(tmpf)) || (!prefsManager->appPrefs.AvailFonts[prefsManager->appPrefs.GFontSub[tmpf]]->usable()))
		{
			newReplacement = true;
			ReplacedFonts.insert(tmpf, prefsManager->appPrefs.toolSettings.defFont);
		}
		else
			ReplacedFonts.insert(tmpf, prefsManager->appPrefs.GFontSub[tmpf]);
	}
	else
	{
		if (!doc->UsedFonts.contains(tmpf))
		{
			doc->AddFont(tmpf, qRound(doc->toolSettings.defSize / 10.0));
		}
	}
	int size = qRound(it->attribute("CSIZE").toDouble() * 10);
	QString fcolor = it->attribute("CCOLOR");
	int extra;
	if (it->hasAttribute("CEXTRA"))
		extra = qRound(it->attribute("CEXTRA").toDouble() / it->attribute("CSIZE").toDouble() * 1000.0);
	else
		extra = it->attribute("CKERN").toInt();
	int shade = it->attribute("CSHADE").toInt();
	int style = it->attribute("CSTYLE").toInt();
	int ab = it->attribute("CAB", "0").toInt();
	QString stroke = it->attribute("CSTROKE", CommonStrings::None);
	int shade2 = it->attribute("CSHADE2", "100").toInt();
	int scale = qRound(it->attribute("CSCALE", "100").toDouble() * 10);
	int scalev = qRound(it->attribute("CSCALEV", "100").toDouble() * 10);
	int base = qRound(it->attribute("CBASE", "0").toDouble() * 10);
	int shX = qRound(it->attribute("CSHX", "5").toDouble() * 10);
	int shY = qRound(it->attribute("CSHY", "-5").toDouble() * 10);
	int outL = qRound(it->attribute("COUT", "1").toDouble() * 10);
	int ulp = qRound(it->attribute("CULP", "-0.1").toDouble() * 10);
	int ulw = qRound(it->attribute("CULW", "-0.1").toDouble() * 10);
	int stp = qRound(it->attribute("CSTP", "-0.1").toDouble() * 10);
	int stw = qRound(it->attribute("CSTW", "-0.1").toDouble() * 10);
	int iobj = it->attribute("COBJ", "-1").toInt();
	for (uint cxx=0; cxx<tmp2.length(); ++cxx)
	{
		CharStyle newStyle;
		QChar ch = tmp2.at(cxx);
		if (ch == QChar(5))
			ch = QChar(13);
		if (ch == QChar(4))
			ch = QChar(9);
		if (unknown)
			newStyle.cfont = dummy;
		else
			newStyle.cfont = (*doc->AllFonts)[tmpf];
		newStyle.csize = size;
		newStyle.ccolor = fcolor;
		newStyle.cextra = extra;
		newStyle.cshade = shade;
		newStyle.cstyle = static_cast<StyleFlag>(style);
		if (ab >= 5) breakPoint();
		if (impo)
		{
			if (VorLFound)
				last->ParaStyle = DoVorl[ab].toUInt();
			else
			{
				if (ab < 5)
					last->ParaStyle = ab;
				else
					last->ParaStyle = 0;
			}
		}
		else
			last->ParaStyle = ab;
		newStyle.cstroke = stroke;
		newStyle.cshade2 = shade2;
		newStyle.cscale = QMIN(QMAX(scale, 100), 4000);
		newStyle.cscalev = QMIN(QMAX(scalev, 100), 4000);
		newStyle.cbase = base;
		newStyle.cshadowx = shX;
		newStyle.cshadowy = shY;
		newStyle.coutline = outL;
		newStyle.cunderpos = ulp;
		newStyle.cunderwidth = ulw;
		newStyle.cstrikepos = stp;
		newStyle.cstrikewidth = stw;
		int pos = obj->itemText.length();
		if (ch == SpecialChars::OBJECT) {
			if (iobj != -1) {
				obj->itemText.insertObject(pos, doc->FrameItems.at(iobj));
			}
		}
		else {
			obj->itemText.insertChars(pos, QString(ch));
		}
		if (newStyle != last->Style) {
#ifdef NLS_PROTO
			qDebug(QString("new style at %1: %2 -> %3").arg(pos).arg(last->Style.asString()).arg(newStyle.asString()));
#endif
			obj->itemText.applyStyle(last->StyleStart, pos-last->StyleStart, last->Style);
			last->Style = newStyle;
			last->StyleStart = pos;
		}
		if (ch == SpecialChars::PARSEP) {
			obj->itemText.applyStyle(pos, doc->docParagraphStyles[QMAX(0,last->ParaStyle)]);
		}
	}
	obj->itemText.applyStyle(last->StyleStart, obj->itemText.length()-last->StyleStart, last->Style);
	obj->itemText.applyStyle(obj->itemText.length()-1, doc->docParagraphStyles[QMAX(0,last->ParaStyle)]);
	return;
}

void Scribus13Format::readParagraphStyle(ParagraphStyle& vg, const QDomElement& pg, SCFonts &avail, ScribusDoc *doc)
{
	vg.setName(pg.attribute("NAME"));
	vg.setLineSpacingMode(static_cast<ParagraphStyle::LineSpacingMode>(pg.attribute("LINESPMode", "0").toInt()));
	vg.setLineSpacing(pg.attribute("LINESP").toDouble());
	vg.setLeftMargin(pg.attribute("INDENT", "0").toDouble());
	if (pg.hasAttribute("RMARGIN"))
		vg.setRightMargin(pg.attribute("RMARGIN", "0").toDouble());
	else
		vg.setRightMargin(0);
	vg.setFirstIndent(pg.attribute("FIRST", "0").toDouble());
	vg.setAlignment(pg.attribute("ALIGN").toInt());
	vg.setGapBefore(pg.attribute("VOR", "0").toDouble());
	vg.setGapAfter(pg.attribute("NACH", "0").toDouble());
	PrefsManager * prefsManager = PrefsManager::instance();
	QString tmpf = pg.attribute("FONT", doc->toolSettings.defFont);
	if ((!avail.find(tmpf)) || (!avail[tmpf]->usable()))
	{
		if ((!prefsManager->appPrefs.GFontSub.contains(tmpf)) || (!avail[prefsManager->appPrefs.GFontSub[tmpf]]->usable()))
		{
			newReplacement = true;
			ReplacedFonts.insert(tmpf, prefsManager->appPrefs.toolSettings.defFont);
		}
		else
			ReplacedFonts.insert(tmpf, prefsManager->appPrefs.GFontSub[tmpf]);
		}
		else
		{
			if (!doc->UsedFonts.contains(tmpf))
			{
				//						QFont fo = avail[tmpf]->Font;
				//						fo.setPointSize(qRound(doc->toolSettings.defSize / 10.0));
				doc->AddFont(tmpf, qRound(doc->toolSettings.defSize / 10.0));
			}
		}
		vg.charStyle().cfont = (*m_Doc->AllFonts)[tmpf];
		vg.charStyle().csize = qRound(pg.attribute("FONTSIZE", "12").toDouble() * 10.0);
		vg.setHasDropCap(static_cast<bool>(pg.attribute("DROP", "0").toInt()));
		vg.setDropCapLines(pg.attribute("DROPLIN", "2").toInt());
		vg.setDropCapOffset(pg.attribute("DROPDIST", "0").toDouble());
		vg.charStyle().cstyle = static_cast<StyleFlag>(pg.attribute("EFFECT", "0").toInt());
		vg.charStyle().ccolor = pg.attribute("FCOLOR", doc->toolSettings.dBrush);
		vg.charStyle().cshade = pg.attribute("FSHADE", "100").toInt();
		vg.charStyle().cstroke = pg.attribute("SCOLOR", doc->toolSettings.dPen);
		vg.charStyle().cshade2 = pg.attribute("SSHADE", "100").toInt();
		vg.setUseBaselineGrid(static_cast<bool>(pg.attribute("BASE", "0").toInt()));
		vg.charStyle().cshadowx = qRound(pg.attribute("TXTSHX", "5").toDouble() * 10);
		vg.charStyle().cshadowy = qRound(pg.attribute("TXTSHY", "-5").toDouble() * 10);
		vg.charStyle().coutline = qRound(pg.attribute("TXTOUT", "1").toDouble() * 10);
		vg.charStyle().cunderpos = qRound(pg.attribute("TXTULP", "-0.1").toDouble() * 10);
		vg.charStyle().cunderwidth = qRound(pg.attribute("TXTULW", "-0.1").toDouble() * 10);
		vg.charStyle().cstrikepos = qRound(pg.attribute("TXTSTP", "-0.1").toDouble() * 10);
		vg.charStyle().cstrikewidth = qRound(pg.attribute("TXTSTW", "-0.1").toDouble() * 10);
		vg.charStyle().cscale = qRound(pg.attribute("SCALEH", "100").toDouble() * 10);
		vg.charStyle().cscalev = qRound(pg.attribute("SCALEV", "100").toDouble() * 10);
		vg.charStyle().cbase = qRound(pg.attribute("BASEO", "0").toDouble() * 10);
		vg.charStyle().cextra = qRound(pg.attribute("KERN", "0").toDouble() * 10);
		vg.tabValues().clear();
		if ((pg.hasAttribute("NUMTAB")) && (pg.attribute("NUMTAB", "0").toInt() != 0))
		{
			ParagraphStyle::TabRecord tb;
			QString tmp = pg.attribute("TABS");
			QTextStream tgv(&tmp, IO_ReadOnly);
			double xf, xf2;
			for (int cxv = 0; cxv < pg.attribute("NUMTAB", "0").toInt(); cxv += 2)
			{
				tgv >> xf;
				tgv >> xf2;
				tb.tabPosition = xf2;
				tb.tabType = static_cast<int>(xf);
				tb.tabFillChar =  QChar();
				vg.tabValues().append(tb);
			}
			tmp = "";
		}
		else
		{
			QDomNode IT = pg.firstChild();
			while(!IT.isNull())
			{
				QDomElement it = IT.toElement();
				if (it.tagName()=="Tabs")
				{
					ParagraphStyle::TabRecord tb;
					tb.tabPosition = it.attribute("Pos").toDouble();
					tb.tabType = it.attribute("Type").toInt();
					QString tbCh = "";
					tbCh = it.attribute("Fill","");
					if (tbCh.isEmpty())
						tb.tabFillChar = QChar();
					else
						tb.tabFillChar = tbCh[0];
					vg.tabValues().append(tb);
				}
				IT=IT.nextSibling();
			}
		}
}

PageItem* Scribus13Format::PasteItem(QDomElement *obj, ScribusDoc *doc)
{
	struct ScImage::LoadRequest loadingInfo;
	int z = 0;
	PageItem::ItemType pt = static_cast<PageItem::ItemType>(obj->attribute("PTYPE").toInt());
	double x = obj->attribute("XPOS").toDouble();
	double y = obj->attribute("YPOS").toDouble();
	double w = obj->attribute("WIDTH").toDouble();
	double h = obj->attribute("HEIGHT").toDouble();
	double pw = obj->attribute("PWIDTH").toDouble();
	double scx = obj->attribute("LOCALSCX").toDouble();
	double scy = obj->attribute("LOCALSCY").toDouble();
	QString Pcolor = obj->attribute("PCOLOR");
	QString Pcolor2 = obj->attribute("PCOLOR2");
	QColor tmpc;
	PageItem *currItem=NULL;
	QString tmp;
	int xi;
	double xf, yf, xf2;
	QString clPath;
	QDomNode IT;
	switch (pt)
	{
	// OBSOLETE CR 2005-02-06
	case PageItem::ItemType1:
		z = doc->itemAdd(PageItem::Polygon, PageItem::Ellipse, x, y, w, h, pw, Pcolor, Pcolor2, true);
		currItem = doc->Items->at(z);
		break;
	//
	case PageItem::ImageFrame:
		z = doc->itemAdd(PageItem::ImageFrame, PageItem::Unspecified, x, y, w, h, 1, doc->toolSettings.dBrushPict, CommonStrings::None, true);
		currItem = doc->Items->at(z);
		currItem->setImageXYScale(scx, scy);
		currItem->setImageXYOffset(obj->attribute("LOCALX").toDouble(), obj->attribute("LOCALY").toDouble());
		currItem->Pfile = obj->attribute("PFILE");
		currItem->IProfile = obj->attribute("PRFILE","");
		currItem->EmProfile = obj->attribute("EPROF","");
		currItem->IRender = obj->attribute("IRENDER", "1").toInt();
		currItem->UseEmbedded = obj->attribute("EMBEDDED", "1").toInt();
		currItem->pixm.imgInfo.lowResType = obj->attribute("ImageRes", "1").toInt();
		IT = obj->firstChild();
		while(!IT.isNull())
		{
			QDomElement it = IT.toElement();
			if (it.tagName()=="ImageEffect")
			{
				struct ScImage::imageEffect ef;
				ef.effectParameters = it.attribute("Param");
				ef.effectCode = it.attribute("Code").toInt();
				currItem->effectsInUse.append(ef);
			}
			IT=IT.nextSibling();
		}
		if (!currItem->Pfile.isEmpty())
			doc->loadPict(currItem->Pfile, currItem, false);
		currItem->IProfile = obj->attribute("PRFILE","");
		currItem->EmProfile = obj->attribute("EPROF","");
		currItem->IRender = obj->attribute("IRENDER", "1").toInt();
		currItem->UseEmbedded = obj->attribute("EMBEDDED", "1").toInt();
		currItem->setImageXYScale(scx, scy);
		clPath = obj->attribute("ImageClip", "");
		if (currItem->pixm.imgInfo.PDSpathData.contains(clPath))
		{
			currItem->imageClip = currItem->pixm.imgInfo.PDSpathData[clPath].copy();
			currItem->pixm.imgInfo.usedPath = clPath;
			QWMatrix cl;
			cl.translate(currItem->imageXOffset()*currItem->imageXScale(), currItem->imageYOffset()*currItem->imageYScale());
			cl.scale(currItem->imageXScale(), currItem->imageYScale());
			currItem->imageClip.map(cl);
		}
		currItem->setImageShown(obj->attribute("PICART").toInt());
/*		currItem->BBoxX = obj->attribute("BBOXX").toDouble();
		currItem->BBoxH = obj->attribute("BBOXH").toDouble(); */
		currItem->ScaleType = obj->attribute("SCALETYPE", "1").toInt();
		currItem->AspectRatio = obj->attribute("RATIO", "0").toInt();
		currItem->setLineWidth(pw);
		if (currItem->pixm.imgInfo.layerInfo.count() != 0)
		{
			bool found = false;
			IT = obj->firstChild();
			while(!IT.isNull())
			{
				QDomElement it = IT.toElement();
				if (it.tagName() == "PSDLayer")
				{
					found = true;
					loadingInfo.blend = it.attribute("Blend");
					loadingInfo.opacity = it.attribute("Opacity").toInt();
					loadingInfo.visible = static_cast<bool>(it.attribute("Visible").toInt());
					currItem->pixm.imgInfo.RequestProps.insert(it.attribute("Layer").toInt(), loadingInfo);
				}
				IT=IT.nextSibling();
			}
			if (found)
			{
				currItem->pixm.imgInfo.isRequest = true;
				doc->loadPict(currItem->Pfile, currItem, true);
			}
		}
		break;
	// OBSOLETE CR 2005-02-06
	case PageItem::ItemType3:
		z = doc->itemAdd(PageItem::Polygon, PageItem::Rectangle, x, y, w, h, pw, Pcolor, Pcolor2, true);
		currItem = doc->Items->at(z);
		break;
	//
	case PageItem::PathText:
		z = doc->itemAdd(PageItem::PathText, PageItem::Unspecified, x, y, w, h, pw, CommonStrings::None, Pcolor, true);
		currItem = doc->Items->at(z);
		if ((obj->attribute("ANNOTATION", "0").toInt()) && (static_cast<bool>(obj->attribute("ANICON", "0").toInt())))
		{
			currItem->setImageXYScale(scx, scy);
			currItem->setImageXYOffset(obj->attribute("LOCALX").toDouble(), obj->attribute("LOCALY").toDouble());
			currItem->Pfile = obj->attribute("PFILE");
			currItem->Pfile2 = obj->attribute("PFILE2","");
			currItem->Pfile3 = obj->attribute("PFILE3","");
			currItem->IProfile = obj->attribute("PRFILE","");
			currItem->EmProfile = obj->attribute("EPROF","");
			currItem->IRender = obj->attribute("IRENDER", "1").toInt();
			currItem->UseEmbedded = obj->attribute("EMBEDDED", "1").toInt();
			doc->LoadPict(currItem->Pfile, z);
			currItem->setImageXYScale(scx, scy);
			currItem->setImageShown(obj->attribute("PICART").toInt());
/*			currItem->BBoxX = obj->attribute("BBOXX").toDouble();
			currItem->BBoxH = obj->attribute("BBOXH").toDouble(); */
			currItem->ScaleType = obj->attribute("SCALETYPE", "1").toInt();
			currItem->AspectRatio = obj->attribute("RATIO", "0").toInt();
		}
		//currItem->convertTo(pt);
		break;
	case PageItem::TextFrame:
		z = doc->itemAdd(PageItem::TextFrame, PageItem::Unspecified, x, y, w, h, pw, CommonStrings::None, Pcolor, true);
		currItem = doc->Items->at(z);
		if ((obj->attribute("ANNOTATION", "0").toInt()) && (static_cast<bool>(obj->attribute("ANICON", "0").toInt())))
		{
			currItem->setImageXYScale(scx, scy);
			currItem->setImageXYOffset(obj->attribute("LOCALX").toDouble(), obj->attribute("LOCALY").toDouble());
			currItem->Pfile = obj->attribute("PFILE");
			currItem->Pfile2 = obj->attribute("PFILE2","");
			currItem->Pfile3 = obj->attribute("PFILE3","");
			currItem->IProfile = obj->attribute("PRFILE","");
			currItem->EmProfile = obj->attribute("EPROF","");
			currItem->IRender = obj->attribute("IRENDER", "1").toInt();
			currItem->UseEmbedded = obj->attribute("EMBEDDED", "1").toInt();
			doc->LoadPict(currItem->Pfile, z);
			currItem->setImageXYScale(scx, scy);
			currItem->setImageShown(obj->attribute("PICART").toInt());
/*			currItem->BBoxX = obj->attribute("BBOXX").toDouble();
			currItem->BBoxH = obj->attribute("BBOXH").toDouble(); */
			currItem->ScaleType = obj->attribute("SCALETYPE", "1").toInt();
			currItem->AspectRatio = obj->attribute("RATIO", "0").toInt();
		}
			//currItem->convertTo(pt);
		break;
	case PageItem::Line:
		z = doc->itemAdd(PageItem::Line, PageItem::Unspecified, x, y, w, h, pw, CommonStrings::None, Pcolor2, true);
		currItem = doc->Items->at(z);
		break;
	case PageItem::Polygon:
		z = doc->itemAdd(PageItem::Polygon, PageItem::Unspecified, x, y, w, h, pw, Pcolor, Pcolor2, true);
		currItem = doc->Items->at(z);
		break;
	case PageItem::PolyLine:
		z = doc->itemAdd(PageItem::PolyLine, PageItem::Unspecified, x, y, w, h, pw, Pcolor, Pcolor2, true);
		currItem = doc->Items->at(z);
		break;
	}
	currItem->FrameType = obj->attribute("FRTYPE", "0").toInt();
	currItem->setStartArrowIndex(obj->attribute("startArrowIndex", "0").toInt());
	currItem->setEndArrowIndex(obj->attribute("endArrowIndex", "0").toInt());
	currItem->NamedLStyle = obj->attribute("NAMEDLST", "");
	currItem->isBookmark = obj->attribute("BOOKMARK").toInt();
	if ((currItem->isBookmark) && (doc->BookMarks.count() == 0))
		doc->OldBM = true;
	currItem->setImageFlippedH(obj->attribute("FLIPPEDH").toInt());
	currItem->setImageFlippedV(obj->attribute("FLIPPEDV").toInt());
	currItem->setCornerRadius(obj->attribute("RADRECT", "0").toDouble());
	currItem->ClipEdited = obj->attribute("CLIPEDIT", "0").toInt();
	currItem->setFillColor(Pcolor);
	currItem->setLineColor(Pcolor2);
	currItem->setFillShade(obj->attribute("SHADE").toInt());
	currItem->setLineShade(obj->attribute("SHADE2").toInt());
	ParagraphStyle pstyle;
	pstyle.setLineSpacing(obj->attribute("LINESP").toDouble());
	pstyle.setLineSpacingMode(static_cast<ParagraphStyle::LineSpacingMode>(obj->attribute("LINESPMode", "0").toInt()));
	pstyle.setAlignment(obj->attribute("ALIGN", "0").toInt());
/*	sigh
	pstyle.charStyle().setFontSize(qRound(obj->attribute("ISIZE", "12").toDouble() * 10));
	pstyle.charStyle().setStrokeColor(obj->attribute("TXTSTROKE", CommonStrings::None));
	pstyle.charStyle().setFillColor(obj->attribute("TXTFILL", "Black"));
	pstyle.charStyle().setStrokeShade(obj->attribute("TXTSTRSH", "100").toInt());
	pstyle.charStyle().setFillShade(obj->attribute("TXTFILLSH", "100").toInt());
	pstyle.charStyle().setScaleH(qRound(obj->attribute("TXTSCALE", "100").toDouble() * 10));
	pstyle.charStyle().setScaleV(qRound(obj->attribute("TXTSCALEV", "100").toDouble() * 10));
	pstyle.charStyle().setBaselineOffset(qRound(obj->attribute("TXTBASE", "0").toDouble() * 10));
	pstyle.charStyle().setShadowXOffset(qRound(obj->attribute("TXTSHX", "5").toDouble() * 10));
	pstyle.charStyle().setShadowYOffset(qRound(obj->attribute("TXTSHY", "-5").toDouble() * 10));
	pstyle.charStyle().setOutlineWidth(qRound(obj->attribute("TXTOUT", "1").toDouble() * 10));
	pstyle.charStyle().setUnderlineOffset(qRound(obj->attribute("TXTULP", "-0.1").toDouble() * 10));
	pstyle.charStyle().setUnderlineWidth(qRound(obj->attribute("TXTULW", "-0.1").toDouble() * 10));
	pstyle.charStyle().setStrikethruOffset(qRound(obj->attribute("TXTSTP", "-0.1").toDouble() * 10));
	pstyle.charStyle().setStrikethruWidth(qRound(obj->attribute("TXTSTW", "-0.1").toDouble() * 10));
	pstyle.charStyle().setEffects(obj->attribute("TXTSTYLE", "0").toInt());
*/
	pstyle.charStyle().csize = (qRound(obj->attribute("ISIZE", "12").toDouble() * 10));
	pstyle.charStyle().cstroke = (obj->attribute("TXTSTROKE", CommonStrings::None));
	pstyle.charStyle().ccolor = (obj->attribute("TXTFILL", "Black"));
	pstyle.charStyle().cshade2 = (obj->attribute("TXTSTRSH", "100").toInt());
	pstyle.charStyle().cshade = (obj->attribute("TXTFILLSH", "100").toInt());
	pstyle.charStyle().cscale = (qRound(obj->attribute("TXTSCALE", "100").toDouble() * 10));
	pstyle.charStyle().cscalev = (qRound(obj->attribute("TXTSCALEV", "100").toDouble() * 10));
	pstyle.charStyle().cbase = (qRound(obj->attribute("TXTBASE", "0").toDouble() * 10));
	pstyle.charStyle().cshadowx = (qRound(obj->attribute("TXTSHX", "5").toDouble() * 10));
	pstyle.charStyle().cshadowy = (qRound(obj->attribute("TXTSHY", "-5").toDouble() * 10));
	pstyle.charStyle().coutline = (qRound(obj->attribute("TXTOUT", "1").toDouble() * 10));
	pstyle.charStyle().cunderpos = (qRound(obj->attribute("TXTULP", "-0.1").toDouble() * 10));
	pstyle.charStyle().cunderwidth = (qRound(obj->attribute("TXTULW", "-0.1").toDouble() * 10));
	pstyle.charStyle().cstrikepos = (qRound(obj->attribute("TXTSTP", "-0.1").toDouble() * 10));
	pstyle.charStyle().cstrikewidth = (qRound(obj->attribute("TXTSTW", "-0.1").toDouble() * 10));
	pstyle.charStyle().cstyle = static_cast<StyleFlag>(obj->attribute("TXTSTYLE", "0").toInt());
	currItem->itemText.setDefaultStyle(pstyle);
	currItem->setRotation(obj->attribute("ROT").toDouble());
	currItem->setTextToFrameDist(obj->attribute("EXTRA").toDouble(),
								obj->attribute("REXTRA", "1").toDouble(),
								obj->attribute("TEXTRA", "1").toDouble(),
								obj->attribute("BEXTRA", "1").toDouble());

	currItem->PLineArt = Qt::PenStyle(obj->attribute("PLINEART").toInt());
	currItem->PLineEnd = Qt::PenCapStyle(obj->attribute("PLINEEND", "0").toInt());
	currItem->PLineJoin = Qt::PenJoinStyle(obj->attribute("PLINEJOIN", "0").toInt());
	currItem->setPrintEnabled(obj->attribute("PRINTABLE").toInt());
	currItem->setIsAnnotation(obj->attribute("ANNOTATION", "0").toInt());
	currItem->annotation().setType(obj->attribute("ANTYPE", "0").toInt());
	QString AnName = obj->attribute("ANNAME","");
	if (!AnName.isEmpty())
	{
		if (currItem->itemName() == AnName)
			currItem->AutoName = true;
		else
		{
			currItem->setItemName(AnName);
			currItem->AutoName = false;
		}
	}
	currItem->annotation().setAction(obj->attribute("ANACTION",""));
	currItem->annotation().setE_act(obj->attribute("ANEACT",""));
	currItem->annotation().setX_act(obj->attribute("ANXACT",""));
	currItem->annotation().setD_act(obj->attribute("ANDACT",""));
	currItem->annotation().setFo_act(obj->attribute("ANFOACT",""));
	currItem->annotation().setBl_act(obj->attribute("ANBLACT",""));
	currItem->annotation().setK_act(obj->attribute("ANKACT",""));
	currItem->annotation().setF_act(obj->attribute("ANFACT",""));
	currItem->annotation().setV_act(obj->attribute("ANVACT",""));
	currItem->annotation().setC_act(obj->attribute("ANCACT",""));
	currItem->annotation().setActionType(obj->attribute("ANACTYP", "0").toInt());
	currItem->annotation().setExtern(obj->attribute("ANEXTERN",""));
	if ((!currItem->annotation().Extern().isEmpty()) && (currItem->annotation().ActionType() != 8))
	{
		QFileInfo efp(currItem->annotation().Extern());
		currItem->annotation().setExtern(efp.absFilePath());
	}
	currItem->annotation().setZiel(obj->attribute("ANZIEL", "0").toInt());
	currItem->annotation().setToolTip(obj->attribute("ANTOOLTIP",""));
	currItem->annotation().setRollOver(obj->attribute("ANROLL",""));
	currItem->annotation().setDown(obj->attribute("ANDOWN",""));
	currItem->annotation().setBwid(obj->attribute("ANBWID", "1").toInt());
	currItem->annotation().setBsty(obj->attribute("ANBSTY", "0").toInt());
	currItem->annotation().setFeed(obj->attribute("ANFEED", "1").toInt());
	currItem->annotation().setFlag(obj->attribute("ANFLAG", "0").toInt());
	currItem->annotation().setFont(obj->attribute("ANFONT", "4").toInt());
	currItem->annotation().setFormat(obj->attribute("ANFORMAT", "0").toInt());
	currItem->annotation().setVis(obj->attribute("ANVIS", "0").toInt());
	currItem->annotation().setIsChk(static_cast<bool>(obj->attribute("ANCHK", "0").toInt()));
	currItem->annotation().setAAact(static_cast<bool>(obj->attribute("ANAA", "0").toInt()));
	currItem->annotation().setHTML(static_cast<bool>(obj->attribute("ANHTML", "0").toInt()));
	currItem->annotation().setUseIcons(static_cast<bool>(obj->attribute("ANICON", "0").toInt()));
	currItem->annotation().setChkStil(obj->attribute("ANCHKS", "0").toInt());
	currItem->annotation().setMaxChar(obj->attribute("ANMC", "-1").toInt());
	currItem->annotation().setBorderColor(obj->attribute("ANBCOL", CommonStrings::None));
	currItem->annotation().setIPlace(obj->attribute("ANPLACE", "1").toInt());
	currItem->annotation().setScaleW(obj->attribute("ANSCALE", "0").toInt());
	currItem->TopLine = static_cast<bool>(obj->attribute("TopLine", "0").toInt());
	currItem->LeftLine = static_cast<bool>(obj->attribute("LeftLine", "0").toInt());
	currItem->RightLine = static_cast<bool>(obj->attribute("RightLine", "0").toInt());
	currItem->BottomLine = static_cast<bool>(obj->attribute("BottomLine", "0").toInt());
	currItem->isTableItem = static_cast<bool>(obj->attribute("isTableItem", "0").toInt());
	currItem->TopLinkID =  obj->attribute("TopLINK", "-1").toInt();
	currItem->LeftLinkID =  obj->attribute("LeftLINK", "-1").toInt();
	currItem->RightLinkID =  obj->attribute("RightLINK", "-1").toInt();
	currItem->BottomLinkID =  obj->attribute("BottomLINK", "-1").toInt();
	currItem->PoShow = obj->attribute("PLTSHOW", "0").toInt();
	currItem->BaseOffs = obj->attribute("BASEOF", "0").toDouble();
	if ( obj->hasAttribute("TEXTFLOWMODE") )
		currItem->setTextFlowMode((PageItem::TextFlowMode) obj->attribute("TEXTFLOWMODE", "0").toInt());
	else if ( obj->attribute("TEXTFLOW").toInt() )
	{
		if (obj->attribute("TEXTFLOW2", "0").toInt())
			currItem->setTextFlowMode(PageItem::TextFlowUsesBoundingBox);
		else if (obj->attribute("TEXTFLOW3", "0").toInt())
			currItem->setTextFlowMode(PageItem::TextFlowUsesContourLine);
		else
			currItem->setTextFlowMode(PageItem::TextFlowUsesFrameShape);	
	}
	else
		currItem->setTextFlowMode(PageItem::TextFlowDisabled);
	if (obj->hasAttribute("EXTRAV"))
		currItem->ExtraV = qRound(obj->attribute("EXTRAV", "0").toDouble() / obj->attribute("ISIZE", "12").toDouble() * 1000.0);
	else
		currItem->ExtraV = obj->attribute("TXTKERN").toInt();
	currItem->DashOffset = obj->attribute("DASHOFF", "0.0").toDouble();
	currItem->setReversed(static_cast<bool>(obj->attribute("REVERS", "0").toInt()));
	currItem->setLocked(static_cast<bool>(obj->attribute("LOCK", "0").toInt()));
	currItem->setSizeLocked(static_cast<bool>(obj->attribute("LOCKR", "0").toInt()));
	currItem->setFillTransparency(obj->attribute("TransValue", "0.0").toDouble());
	currItem->fillRule = static_cast<bool>(obj->attribute("fillRule", "1").toInt());
	currItem->doOverprint = static_cast<bool>(obj->attribute("doOverprint", "0").toInt());
	if (obj->hasAttribute("TransValueS"))
		currItem->setLineTransparency(obj->attribute("TransValueS", "0.0").toDouble());
	else
		currItem->setLineTransparency(obj->attribute("TransValue", "0.0").toDouble());
	if (obj->attribute("TRANSPARENT", "0").toInt() == 1)
		currItem->setFillColor(CommonStrings::None);
	currItem->Cols = obj->attribute("COLUMNS", "1").toInt();
	currItem->ColGap = obj->attribute("COLGAP", "0.0").toDouble();
	if (obj->attribute("LAYER", "0").toInt() != -1)
		currItem->LayerNr = obj->attribute("LAYER", "0").toInt();
	tmp = "";
	if ((obj->hasAttribute("GROUPS")) && (obj->attribute("NUMGROUP", "0").toInt() != 0))
	{
		tmp = obj->attribute("GROUPS");
		QTextStream fg(&tmp, IO_ReadOnly);
		currItem->Groups.clear();
		for (int cx = 0; cx < obj->attribute("NUMGROUP", "0").toInt(); ++cx)
		{
			fg >> xi;
			currItem->Groups.push(xi);
		}
		tmp = "";
	}
	else
		currItem->Groups.clear();
	tmp = "";
	currItem->TabValues.clear();
	if ((obj->hasAttribute("NUMTAB")) && (obj->attribute("NUMTAB", "0").toInt() != 0))
	{
		ParagraphStyle::TabRecord tb;
		tmp = obj->attribute("TABS");
		QTextStream tgv(&tmp, IO_ReadOnly);
		for (int cxv = 0; cxv < obj->attribute("NUMTAB", "0").toInt(); cxv += 2)
		{
			tgv >> xf;
			tgv >> xf2;
			tb.tabPosition = xf2;
			tb.tabType = static_cast<int>(xf);
			tb.tabFillChar = QChar();
			currItem->TabValues.append(tb);
		}
		tmp = "";
	}
	else
	{
		IT = obj->firstChild();
		while(!IT.isNull())
		{
			QDomElement it = IT.toElement();
			if (it.tagName()=="Tabs")
			{
				ParagraphStyle::TabRecord tb;
				tb.tabPosition = it.attribute("Pos").toDouble();
				tb.tabType = it.attribute("Type").toInt();
				QString tbCh = "";
				tbCh = it.attribute("Fill","");
				if (tbCh.isEmpty())
					tb.tabFillChar = QChar();
				else
					tb.tabFillChar = tbCh[0];
				currItem->TabValues.append(tb);
			}
			IT=IT.nextSibling();
		}
	}
	if ((obj->hasAttribute("NUMDASH")) && (obj->attribute("NUMDASH", "0").toInt() != 0))
	{
		tmp = obj->attribute("DASHS");
		QTextStream dgv(&tmp, IO_ReadOnly);
		currItem->DashValues.clear();
		for (int cxv = 0; cxv < obj->attribute("NUMDASH", "0").toInt(); ++cxv)
		{
			dgv >> xf;
			currItem->DashValues.append(xf);
		}
		tmp = "";
	}
	else
		currItem->DashValues.clear();
	tmp = "";
	if (obj->hasAttribute("NUMPO"))
	{
		currItem->PoLine.resize(obj->attribute("NUMPO").toUInt());
		tmp = obj->attribute("POCOOR");
		QTextStream fp(&tmp, IO_ReadOnly);
		for (uint cx=0; cx<obj->attribute("NUMPO").toUInt(); ++cx)
		{
			fp >> xf;
			fp >> yf;
			currItem->PoLine.setPoint(cx, xf, yf);
		}
	}
	else
		currItem->PoLine.resize(0);
	tmp = "";
	if (obj->hasAttribute("NUMCO"))
	{
		currItem->ContourLine.resize(obj->attribute("NUMCO").toUInt());
		tmp = obj->attribute("COCOOR");
		QTextStream fp(&tmp, IO_ReadOnly);
		for (uint cx=0; cx<obj->attribute("NUMCO").toUInt(); ++cx)
		{
			fp >> xf;
			fp >> yf;
			currItem->ContourLine.setPoint(cx, xf, yf);
		}
	}
	else
		currItem->ContourLine = currItem->PoLine.copy();
	if (!currItem->asLine())
		currItem->Clip = FlattenPath(currItem->PoLine, currItem->Segments);
	else
	{
		int ph = static_cast<int>(QMAX(1.0, currItem->lineWidth() / 2.0));
		currItem->Segments.clear();
		currItem->PoLine.resize(0);
		currItem->Clip.setPoints(4, -ph,-ph, static_cast<int>(currItem->width()+ph),-ph,
		                  static_cast<int>(currItem->width()+ph),static_cast<int>(currItem->height()+ph),
		                  -ph,static_cast<int>(currItem->height()+ph));
		currItem->setHeight(1.0);
	}
	if (currItem->asImageFrame())
		currItem->AdjustPictScale();
	if (currItem->asPathText())
	{
		currItem->updatePolyClip();
		currItem->Frame = true;
	}
	currItem->GrType = obj->attribute("GRTYP", "0").toInt();
	QString GrColor;
	QString GrColor2;
	int GrShade;
	int GrShade2;
	if (currItem->GrType != 0)
	{
		currItem->GrStartX = obj->attribute("GRSTARTX", "0.0").toDouble();
		currItem->GrStartY = obj->attribute("GRSTARTY", "0.0").toDouble();
		currItem->GrEndX = obj->attribute("GRENDX", "0.0").toDouble();
		currItem->GrEndY = obj->attribute("GRENDY", "0.0").toDouble();
		GrColor = obj->attribute("GRCOLOR","");
		if (!GrColor.isEmpty())
		{
			GrColor2 = obj->attribute("GRCOLOR2","");
			GrShade = obj->attribute("GRSHADE", "100").toInt();
			GrShade2 = obj->attribute("GRSHADE2", "100").toInt();
		}
	}
	if (currItem->GrType != 0)
	{
		currItem->fill_gradient.clearStops();
		if ((!GrColor.isEmpty()) && (!GrColor2.isEmpty()))
		{
			if (currItem->GrType == 5)
			{
				if ((GrColor != CommonStrings::None) && (!GrColor.isEmpty()))
					currItem->SetFarbe(&tmpc, GrColor, GrShade);
				currItem->fill_gradient.addStop(tmpc, 0.0, 0.5, 1.0, GrColor, GrShade);
				if ((GrColor2 != CommonStrings::None) && (!GrColor2.isEmpty()))
					currItem->SetFarbe(&tmpc, GrColor2, GrShade2);
				currItem->fill_gradient.addStop(tmpc, 1.0, 0.5, 1.0, GrColor2, GrShade2);
			}
			else
			{
				if ((GrColor2 != CommonStrings::None) && (!GrColor2.isEmpty()))
					currItem->SetFarbe(&tmpc, GrColor2, GrShade2);
				currItem->fill_gradient.addStop(tmpc, 0.0, 0.5, 1.0, GrColor2, GrShade2);
				if ((GrColor != CommonStrings::None) && (!GrColor.isEmpty()))
					currItem->SetFarbe(&tmpc, GrColor, GrShade);
				currItem->fill_gradient.addStop(tmpc, 1.0, 0.5, 1.0, GrColor, GrShade);
			}
		}
		currItem->updateGradientVectors();
	}
	//currItem->setRedrawBounding();
	//currItem->OwnPage = view->OnPage(currItem);
	return currItem;
}

bool Scribus13Format::loadPage(const QString & fileName, int pageNumber, bool Mpage, QString renamedPageName)
{
	qDebug(QString("loading page %2 from file '%1' from 1.3.x plugin").arg(fileName).arg(pageNumber));
	if (m_Doc==0 || m_AvailableFonts==0 || m_mwProgressBar==0)
	{
		Q_ASSERT(m_Doc==0 || m_AvailableFonts==0 || m_mwProgressBar==0);
		return false;
	}
	ParagraphStyle vg;
	struct Layer la;
	struct ScribusDoc::BookMa bok;
	PageItem *Neu;
	Page* Apage;
	LFrames.clear();
	QString tmV, tmp, tmpf, tmp2, tmp3, tmp4, PgNam, Defont, tmf;
	QFont fo;
	QMap<int,int> TableID;
	QPtrList<PageItem> TableItems;
	int a, counter, baseobj;
	double pageX, pageY;
	bool newVersion = false;
	bool VorLFound = false;
	QMap<int,int> layerTrans;
	int maxLayer = 0;
	int maxLevel = 0;
	layerTrans.clear();
	uint layerCount=m_Doc->layerCount();
	for (uint la2 = 0; la2 < layerCount; ++la2)
	{
		maxLayer = QMAX(m_Doc->Layers[la2].LNr, maxLayer);
		maxLevel = QMAX(m_Doc->Layers[la2].Level, maxLevel);
	}
	DoVorl.clear();
	DoVorl[0] = "0";
	DoVorl[1] = "1";
	DoVorl[2] = "2";
	DoVorl[3] = "3";
	DoVorl[4] = "4";
	VorlC = 5;
	QDomDocument docu("scridoc");
 	QString f(readSLA(fileName));
	if (f.isEmpty())
		return false;
	if(!docu.setContent(f))
		return false;
	ScColor lf = ScColor();
	QDomElement elem=docu.documentElement();
	if (elem.tagName() != "SCRIBUSUTF8NEW")
		return false;
	if (elem.hasAttribute("Version"))
		newVersion = true;
	QDomNode DOC=elem.firstChild();
	counter = m_Doc->Items->count();
	baseobj = counter;
	PrefsManager* prefsManager=PrefsManager::instance();
	while(!DOC.isNull())
	{
		QDomElement dc=DOC.toElement();
		QDomNode PAGE=DOC.firstChild();
		while(!PAGE.isNull())
		{
			QDomElement pg=PAGE.toElement();
			if(pg.tagName()=="COLOR" && pg.attribute("NAME")!=CommonStrings::None)
			{
				if (pg.hasAttribute("CMYK"))
					lf.setNamedColor(pg.attribute("CMYK"));
				else
					lf.fromQColor(QColor(pg.attribute("RGB")));
				if (pg.hasAttribute("Spot"))
					lf.setSpotColor(static_cast<bool>(pg.attribute("Spot").toInt()));
				else
					lf.setSpotColor(false);
				if (pg.hasAttribute("Register"))
					lf.setRegistrationColor(static_cast<bool>(pg.attribute("Register").toInt()));
				else
					lf.setRegistrationColor(false);
			  m_Doc->PageColors[pg.attribute("NAME")] = lf;
			}
			if(pg.tagName()=="STYLE")
			{
				GetStyle(&pg, &vg, m_Doc->docParagraphStyles, m_Doc, true);
				VorLFound = true;
			}
			if(pg.tagName()=="JAVA")
				m_Doc->JavaScripts[pg.attribute("NAME")] = pg.attribute("SCRIPT");
			if(pg.tagName()=="LAYERS")
			{
				la.LNr = pg.attribute("NUMMER").toInt();
				la.Level = pg.attribute("LEVEL").toInt();
				la.Name = pg.attribute("NAME");
				la.isViewable = pg.attribute("SICHTBAR").toInt();
				la.isPrintable = pg.attribute("DRUCKEN").toInt();
				la.isEditable = pg.attribute("EDIT", "1").toInt();
				la.flowControl = pg.attribute("FLOW", "1").toInt();
				la.transparency = pg.attribute("TRANS", "1").toDouble();
				la.blendMode = pg.attribute("BLEND", "0").toInt();
				la.outlineMode = pg.attribute("OUTL", "0").toInt();
				if (pg.hasAttribute("LAYERC"))
					la.markerColor =  QColor(pg.attribute("LAYERC","#000000"));
				else
				{
					QColor marker;
					switch (la.LNr % 7)
					{
						case 0:
							marker = Qt::black;
							break;
						case 1:
							marker = Qt::red;
							break;
						case 2:
							marker = Qt::green;
							break;
						case 3:
							marker = Qt::blue;
							break;
						case 4:
							marker = Qt::cyan;
							break;
						case 5:
							marker = Qt::magenta;
							break;
						case 6:
							marker = Qt::yellow;;
							break;
					}
					la.markerColor = marker;
				}
				bool laex = false;
				uint layerCount=m_Doc->layerCount();
				for (uint la2 = 0; la2 < layerCount; ++la2)
				{
					if (m_Doc->Layers[la2].Name == la.Name)
					{
						laex = true;
						layerTrans.insert(la.LNr, m_Doc->Layers[la2].LNr);
					}
				}
				if (!laex)
				{
					maxLayer++;
					maxLevel++;
					layerTrans.insert(la.LNr, maxLayer);
					la.LNr = maxLayer;
					la.Level = maxLevel;
					m_Doc->Layers.append(la);
				}
			}
			if(pg.tagName()=="MultiLine")
			{
				multiLine ml;
				QDomNode MuLn = PAGE.firstChild();
				while(!MuLn.isNull())
				{
					QDomElement MuL = MuLn.toElement();
					struct SingleLine sl;
					sl.Color = MuL.attribute("Color");
					sl.Dash = MuL.attribute("Dash").toInt();
					sl.LineEnd = MuL.attribute("LineEnd").toInt();
					sl.LineJoin = MuL.attribute("LineJoin").toInt();
					sl.Shade = MuL.attribute("Shade").toInt();
					sl.Width = MuL.attribute("Width").toDouble();
					ml.push_back(sl);
					MuLn = MuLn.nextSibling();
				}
				QString Nam = pg.attribute("Name");
				QString Nam2 = Nam;
				int copyC = 1;
				while (m_Doc->MLineStyles.contains(Nam2))
				{
					Nam2 = QObject::tr("Copy #%1 of ").arg(copyC)+Nam;
					copyC++;
				}
				m_Doc->MLineStyles.insert(Nam2, ml);
			}
			if(pg.tagName()=="Arrows")
			{
				struct ArrowDesc arrow;
				arrow.name = pg.attribute("Name");
				arrow.userArrow = true;
				double xa, ya;
				QString tmp = pg.attribute("Points");
				QTextStream fp(&tmp, IO_ReadOnly);
				for (uint cx = 0; cx < pg.attribute("NumPoints").toUInt(); ++cx)
				{
					fp >> xa;
					fp >> ya;
					arrow.points.addPoint(xa, ya);
				}
				m_Doc->arrowStyles.append(arrow);
			}
			if (((pg.tagName()=="PAGE") || (pg.tagName()=="MASTERPAGE")) && (pg.attribute("NUM").toInt() == pageNumber))
			{
				a = m_Doc->currentPage()->pageNr();
				Apage = m_Doc->Pages->at(a);
				if ((pg.tagName()!="MASTERPAGE") && (Mpage))
				{
					PAGE=PAGE.nextSibling();
					continue;
				}
				if (Mpage)
				{
					Apage->LeftPg=pg.attribute("LEFT", "0").toInt();
					
					if (!renamedPageName.isEmpty())
						Apage->setPageName(renamedPageName);
					else
						Apage->setPageName(pg.attribute("NAM",""));
				}
				if (pg.hasAttribute("Size"))
					Apage->PageSize = pg.attribute("Size");
				if (pg.hasAttribute("Orientation"))
					Apage->PageOri = pg.attribute("Orientation").toInt();
				if (pg.hasAttribute("PAGEWIDTH"))
					Apage->setWidth(pg.attribute("PAGEWIDTH").toDouble());
				else
					Apage->setWidth(pg.attribute("PAGEWITH").toDouble());
				Apage->setHeight(pg.attribute("PAGEHEIGHT").toDouble());
				Apage->setInitialHeight(Apage->height());
				Apage->setInitialWidth(Apage->width());
				Apage->initialMargins.Top = QMAX(0.0, pg.attribute("BORDERTOP").toDouble());
				Apage->initialMargins.Bottom = QMAX(0.0, pg.attribute("BORDERBOTTOM").toDouble());
				Apage->initialMargins.Left = QMAX(0.0, pg.attribute("BORDERLEFT").toDouble());
				Apage->initialMargins.Right = QMAX(0.0, pg.attribute("BORDERRIGHT").toDouble());
				Apage->Margins.Top = Apage->initialMargins.Top;
				Apage->Margins.Bottom = Apage->initialMargins.Bottom;
				pageX = pg.attribute("PAGEXPOS").toDouble();
				pageY = pg.attribute("PAGEYPOS").toDouble();
				// guides reading
				tmp = "";
				GuideManagerCore::readVerticalGuides(pg.attribute("VerticalGuides"),
												Apage,
												GuideManagerCore::Standard,
												pg.hasAttribute("NumVGuides"));
				GuideManagerCore::readHorizontalGuides(pg.attribute("HorizontalGuides"),
												Apage,
												GuideManagerCore::Standard,
												pg.hasAttribute("NumHGuides"));
			}
			if ((pg.tagName()=="PAGEOBJECT") || (pg.tagName()=="MASTEROBJECT") || (pg.tagName()=="FRAMEOBJECT"))
			{
				if ((pg.tagName()!="MASTEROBJECT") && (Mpage))
				{
					PAGE=PAGE.nextSibling();
					continue;
				}
				if (pg.attribute("OwnPage").toInt() == pageNumber)
				{
					if (pg.attribute("NEXTITEM").toInt() != -1)
					{
						if (pg.attribute("BACKITEM").toInt() == -1)
							LFrames.append(m_Doc->Items->count());
					}
					int docGc = m_Doc->GroupCounter;
					m_Doc->GroupCounter = 0;
					Neu = PasteItem(&pg, m_Doc);
					Neu->moveBy(-pageX + Apage->xOffset(), - pageY + Apage->yOffset());
					Neu->setRedrawBounding();
					//CB Must run onpage as we cant use pagetoload if the page has been renamed. 
					//CB TODO Make this accept a page to place onto.
					//Neu->OwnPage = m_Doc->OnPage(Neu);
					//Neu->OwnPage = PageToLoad;
					Neu->OwnPage = m_Doc->currentPageNumber();
					if (pg.tagName()=="PAGEOBJECT")
						Neu->OnMasterPage = "";
					m_Doc->GroupCounter = docGc;
					tmpf = pg.attribute("IFONT", m_Doc->toolSettings.defFont);
					if ((!m_AvailableFonts->find(tmpf)) || (!(*m_AvailableFonts)[tmpf]->usable()))
					{
						if ((!prefsManager->appPrefs.GFontSub.contains(tmpf)) || (!(*m_AvailableFonts)[prefsManager->appPrefs.GFontSub[tmpf]]->usable()))
						{
							newReplacement = true;
							ReplacedFonts.insert(tmpf, prefsManager->appPrefs.toolSettings.defFont);
						}
						else
							ReplacedFonts.insert(tmpf, prefsManager->appPrefs.GFontSub[tmpf]);
					}
					else
					{
						if (!m_Doc->UsedFonts.contains(tmpf))
						{
//							QFont fo = avail[tmpf]->Font;
//							fo.setPointSize(qRound(m_Doc->toolSettings.defSize / 10.0));
							m_Doc->AddFont(tmpf, qRound(m_Doc->toolSettings.defSize / 10.0));
						}
					}
					QDomNode IT=pg.firstChild();
					LastStyles * last = new LastStyles();
					while(!IT.isNull())
					{
						QDomElement it=IT.toElement();
						if (it.tagName()=="CSTOP")
						{
							QString name = it.attribute("NAME");
							double ramp = it.attribute("RAMP", "0.0").toDouble();
							int shade = it.attribute("SHADE", "100").toInt();
							double opa = it.attribute("TRANS", "1").toDouble();
							Neu->fill_gradient.addStop(SetColor(m_Doc, name, shade), ramp, 0.5, opa, name, shade);
						}
						if (it.tagName()=="ITEXT")
							GetItemText(&it, m_Doc, Neu, last, true, VorLFound);
						if(it.tagName()=="PageItemAttributes")
						{
							QDomNode PIA = it.firstChild();
							ObjAttrVector pageItemAttributes;
							while(!PIA.isNull())
							{
								QDomElement itemAttr = PIA.toElement();
								if(itemAttr.tagName() == "ItemAttribute")
								{
									ObjectAttribute objattr;
									objattr.name=itemAttr.attribute("Name");
									objattr.type=itemAttr.attribute("Type");
									objattr.value=itemAttr.attribute("Value");
									objattr.parameter=itemAttr.attribute("Parameter");
									objattr.relationship=itemAttr.attribute("Relationship");
									objattr.relationshipto=itemAttr.attribute("RelationshipTo");
									objattr.autoaddto=itemAttr.attribute("AutoAddTo");
									pageItemAttributes.append(objattr);
								}
								PIA = PIA.nextSibling();
							}
							Neu->setObjectAttributes(&pageItemAttributes);
						}
						IT=IT.nextSibling();
					}
					delete last;
					if (Neu->fill_gradient.Stops() == 0)
					{
						Neu->fill_gradient.addStop(m_Doc->PageColors[m_Doc->toolSettings.dBrush].getRGBColor(), 0.0, 0.5, 1.0, m_Doc->toolSettings.dBrush, 100);
						Neu->fill_gradient.addStop(m_Doc->PageColors[m_Doc->toolSettings.dPen].getRGBColor(), 1.0, 0.5, 1.0, m_Doc->toolSettings.dPen, 100);
					}
//					Neu->Language = ScMW->GetLang(pg.attribute("LANGUAGE", m_Doc->Language));
//					Neu->Language = m_Doc->Language;
					Neu->isAutoText = static_cast<bool>(pg.attribute("AUTOTEXT").toInt());
					Neu->isEmbedded = static_cast<bool>(pg.attribute("isInline", "0").toInt());
					Neu->gXpos = pg.attribute("gXpos", "0.0").toDouble();
					Neu->gYpos = pg.attribute("gYpos", "0.0").toDouble();
					QString defaultVal;
					defaultVal.setNum(Neu->width());
					Neu->gWidth = pg.attribute("gWidth",defaultVal).toDouble();
					defaultVal.setNum(Neu->height());
					Neu->gHeight = pg.attribute("gHeight",defaultVal).toDouble();
/*FIXME					if (Neu->lineSpacingMode() == 3)
					{
						m_Doc->docParagraphStyles[0].setUseBaselineGrid(true);
						Neu->setLineSpacing(m_Doc->typographicSettings.valueBaseGrid-1);
					}
*/					if (Neu->isAutoText)
						m_Doc->LastAuto = Neu;
					Neu->NextIt = baseobj + pg.attribute("NEXTITEM").toInt();
					if (Neu->isTableItem)
					{
						TableItems.append(Neu);
						TableID.insert(pg.attribute("OwnLINK", "0").toInt(), Neu->ItemNr);
					}
					if (pg.tagName()=="FRAMEOBJECT")
					{
						m_Doc->FrameItems.append(m_Doc->Items->take(Neu->ItemNr));
						Neu->ItemNr = m_Doc->FrameItems.count()-1;
					}
				}
				counter++;
			}
			PAGE=PAGE.nextSibling();
		}
		PAGE=DOC.firstChild();
		while(!PAGE.isNull())
		{
			QDomElement pg=PAGE.toElement();
			if(pg.tagName()=="Bookmark")
			{
				bok.Title = pg.attribute("Title");
				bok.Text = pg.attribute("Text");
				bok.Aktion = pg.attribute("Aktion");
				bok.ItemNr = pg.attribute("ItemNr").toInt();
				bok.PageObject = m_Doc->Items->at(pg.attribute("Element").toInt());
				bok.First = pg.attribute("First").toInt();
				bok.Last = pg.attribute("Last").toInt();
				bok.Prev = pg.attribute("Prev").toInt();
				bok.Next = pg.attribute("Next").toInt();
				bok.Parent = pg.attribute("Parent").toInt();
				m_Doc->BookMarks.append(bok);
			}
			PAGE=PAGE.nextSibling();
		}
		DOC=DOC.nextSibling();
	}
	if (TableItems.count() != 0)
	{
		for (uint ttc = 0; ttc < TableItems.count(); ++ttc)
		{
			PageItem* ta = TableItems.at(ttc);
			if (ta->TopLinkID != -1)
				ta->TopLink = m_Doc->Items->at(TableID[ta->TopLinkID]);
			else
				ta->TopLink = 0;
			if (ta->LeftLinkID != -1)
				ta->LeftLink = m_Doc->Items->at(TableID[ta->LeftLinkID]);
			else
				ta->LeftLink = 0;
			if (ta->RightLinkID != -1)
				ta->RightLink = m_Doc->Items->at(TableID[ta->RightLinkID]);
			else
				ta->RightLink = 0;
			if (ta->BottomLinkID != -1)
				ta->BottomLink = m_Doc->Items->at(TableID[ta->BottomLinkID]);
			else
				ta->BottomLink = 0;
		}
	}
	if (LFrames.count() != 0)
	{
		PageItem *Its;
		PageItem *Itn;
		PageItem *Itr;
		QValueList<int>::Iterator lc;
		for (lc = LFrames.begin(); lc != LFrames.end(); ++lc)
		{
			Its = m_Doc->Items->at((*lc));
			Itr = Its;
			Its->BackBox = 0;
			while (Its->NextIt != -1)
			{
				if (Its->NextIt < static_cast<int>(m_Doc->Items->count()))
				{
					Itn = m_Doc->Items->at(Its->NextIt);
					Its->NextBox = Itn;
					Itn->BackBox = Its;
					Its = Itn;
				}
				else
					Its->NextIt = -1;
			}
			Its->NextBox = 0;
		}
	}
	return true;
}

void Scribus13Format::GetStyle(QDomElement *pg, ParagraphStyle *vg, QValueList<ParagraphStyle> &docParagraphStyles, ScribusDoc* doc, bool fl)
{
	bool fou;
	QString tmpf, tmf, tmV;
	fou = false;
	PrefsManager* prefsManager=PrefsManager::instance();
	readParagraphStyle(*vg, *pg, prefsManager->appPrefs.AvailFonts, doc);
	for (uint xx=0; xx<docParagraphStyles.count(); ++xx)
	{
		if (vg->name() == docParagraphStyles[xx].name())
		{
			if (vg->equiv(docParagraphStyles[xx]))
			{
				if (fl)
				{
					DoVorl[VorlC] = tmV.setNum(xx);
					VorlC++;
				}
				fou = true;
			}
			else
			{
				vg->setName("Copy of "+docParagraphStyles[xx].name());
				fou = false;
			}
			break;
		}
	}
	if (!fou)
	{
		for (uint xx=0; xx< docParagraphStyles.count(); ++xx)
		{
			if (vg->equiv(docParagraphStyles[xx]))
			{
				vg->setName(docParagraphStyles[xx].name());
				fou = true;
				if (fl)
				{
					DoVorl[VorlC] = tmV.setNum(xx);
					VorlC++;
				}
				break;
			}
		}
	}
	if (!fou)
	{
		docParagraphStyles.append(*vg);
		if (fl)
		{
			DoVorl[VorlC] = tmV.setNum(docParagraphStyles.count()-1);
			VorlC++;
		}
	}
}

QString Scribus13Format::AskForFont(SCFonts &avail, QString fStr, ScribusDoc *doc)
{
	PrefsManager *prefsManager=PrefsManager::instance();
//	QFont fo;
	QString tmpf = fStr;
	if ((!avail.find(tmpf)) || (!avail[tmpf]->usable()))
	{
		if ((!prefsManager->appPrefs.GFontSub.contains(tmpf)) || (!avail[prefsManager->appPrefs.GFontSub[tmpf]]->usable()))
		{
			qApp->setOverrideCursor(QCursor(arrowCursor), true);
			MissingFont *dia = new MissingFont(0, tmpf, doc);
			dia->exec();
			tmpf = dia->getReplacementFont();
			delete dia;
			qApp->setOverrideCursor(QCursor(waitCursor), true);
			prefsManager->appPrefs.GFontSub[fStr] = tmpf;
		}
		else
			tmpf = prefsManager->appPrefs.GFontSub[tmpf];
		ReplacedFonts[fStr] = tmpf;
	}
	if (!doc->UsedFonts.contains(tmpf))
	{
//		fo = avail[tmpf]->Font;
//		fo.setPointSize(qRound(doc->toolSettings.defSize / 10.0));
		doc->AddFont(tmpf);
	}
// 	DoFonts[fStr] = tmpf;
	return tmpf;
}

bool Scribus13Format::readStyles(const QString& fileName, ScribusDoc* doc, QValueList<ParagraphStyle> &docParagraphStyles)
{
	ParagraphStyle vg;
	QDomDocument docu("scridoc");
	QString tmpf, tmf;
	QString f (readSLA(fileName));
	if (f.isEmpty())
		return false;
	if(!docu.setContent(f))
		return false;
	QDomElement elem=docu.documentElement();
	if ((elem.tagName() != "SCRIBUS") && (elem.tagName() != "SCRIBUSUTF8NEW"))
		return false;
	QDomNode DOC=elem.firstChild();
	while(!DOC.isNull())
	{
		QDomElement dc=DOC.toElement();
		QDomNode PAGE=DOC.firstChild();
		while(!PAGE.isNull())
		{
			QDomElement pg=PAGE.toElement();
			if(pg.tagName()=="STYLE")
				GetStyle(&pg, &vg, docParagraphStyles, doc, false);
			PAGE=PAGE.nextSibling();
		}
		DOC=DOC.nextSibling();
	}
	return true;
}

bool Scribus13Format::readLineStyles(const QString& fileName, QMap<QString,multiLine> *Sty)
{
	QDomDocument docu("scridoc");
	QString f(readSLA(fileName));
	if (f.isEmpty())
		return false;
	if(!docu.setContent(f))
		return false;
	QDomElement elem=docu.documentElement();
	if ((elem.tagName() != "SCRIBUS") && (elem.tagName() != "SCRIBUSUTF8NEW"))
		return false;
	QDomNode DOC=elem.firstChild();
	while(!DOC.isNull())
	{
		QDomElement dc=DOC.toElement();
		QDomNode PAGE=DOC.firstChild();
		while(!PAGE.isNull())
		{
			QDomElement pg=PAGE.toElement();
			if(pg.tagName()=="MultiLine")
			{
				multiLine ml;
				QDomNode MuLn = PAGE.firstChild();
				while(!MuLn.isNull())
				{
					QDomElement MuL = MuLn.toElement();
					struct SingleLine sl;
					sl.Color = MuL.attribute("Color");
					sl.Dash = MuL.attribute("Dash").toInt();
					sl.LineEnd = MuL.attribute("LineEnd").toInt();
					sl.LineJoin = MuL.attribute("LineJoin").toInt();
					sl.Shade = MuL.attribute("Shade").toInt();
					sl.Width = MuL.attribute("Width").toDouble();
					ml.push_back(sl);
					MuLn = MuLn.nextSibling();
				}
				QString Nam = pg.attribute("Name");
				QString Nam2 = Nam;
				int copyC = 1;
				while (Sty->contains(Nam2))
				{
					Nam2 = tr("Copy #%1 of ").arg(copyC)+Nam;
					copyC++;
				}
				Sty->insert(Nam2, ml);
			}
			PAGE=PAGE.nextSibling();
		}
		DOC=DOC.nextSibling();
	}
	return true;
}

bool Scribus13Format::readColors(const QString& fileName, ColorList & colors)
{
	QString f(readSLA(fileName));
	if (f.isEmpty())
		return false;
	QDomDocument docu("scridoc");
	if(!docu.setContent(f))
		return false;
	colors.clear();
	ScColor lf = ScColor();
	QDomElement elem=docu.documentElement();
	if ((elem.tagName() != "SCRIBUS") && (elem.tagName() != "SCRIBUSUTF8NEW"))
		return false;
	QDomNode DOC=elem.firstChild();
	while(!DOC.isNull())
	{
		QDomElement dc=DOC.toElement();
		QDomNode PAGE=DOC.firstChild();
		while(!PAGE.isNull())
		{
			QDomElement pg=PAGE.toElement();
			// 10/25/2004 pv - None is "reserved" color. cannot be defined in any file...
			if(pg.tagName()=="COLOR" && pg.attribute("NAME")!=CommonStrings::None)
			{
				if (pg.hasAttribute("CMYK"))
					lf.setNamedColor(pg.attribute("CMYK"));
				else
					lf.fromQColor(QColor(pg.attribute("RGB")));
				if (pg.hasAttribute("Spot"))
					lf.setSpotColor(static_cast<bool>(pg.attribute("Spot").toInt()));
				else
					lf.setSpotColor(false);
				if (pg.hasAttribute("Register"))
					lf.setRegistrationColor(static_cast<bool>(pg.attribute("Register").toInt()));
				else
					lf.setRegistrationColor(false);
				colors[pg.attribute("NAME")] = lf;
			}
			PAGE=PAGE.nextSibling();
		}
		DOC=DOC.nextSibling();
	}
	return true;
}

bool Scribus13Format::readPageCount(const QString& fileName, int *num1, int *num2, QStringList & masterPageNames)
{
	QString PgNam;
	int counter = 0;
	int counter2 = 0;
	QDomDocument docu("scridoc");
	QString f(readSLA(fileName));
	if (f.isEmpty())
		return false;
	if(!docu.setContent(f))
		return false;
	QDomElement elem=docu.documentElement();
	if ((elem.tagName() != "SCRIBUSUTF8NEW"))
		return false;
	QDomNode DOC=elem.firstChild();
	while(!DOC.isNull())
	{
		QDomNode PAGE=DOC.firstChild();
		while(!PAGE.isNull())
		{
			QDomElement pg=PAGE.toElement();
			PgNam = pg.attribute("NAM", "");
			if(pg.tagName()=="PAGE")
				counter++;
			if(pg.tagName()=="MASTERPAGE")
			{
				counter2++;
				masterPageNames.append(PgNam);
			}
			PAGE=PAGE.nextSibling();
		}
		DOC=DOC.nextSibling();
	}
	*num1 = counter;
	*num2 = counter2;
	return true;
}
