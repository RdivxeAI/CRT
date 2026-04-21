#pragma once

#include <QLocale>
#include <QString>
#include <Qt>

enum class AppLanguage {
    Arabic,
    English,
    French,
    Chinese
};

struct UiText {
    QString appTitle;
    QString readyStatus;
    QString selectImageTitle;
    QString imageFileFilter;
    QString imageLoadFailedTitle;
    QString imageLoadFailedBody;
    QString imageLoadedStatus;
    QString selectVectorTitle;
    QString jsonFileFilter;
    QString vectorLoadFailedTitle;
    QString vectorLoadFailedBody;
    QString vectorLoadedTitle;
    QString vectorLoadedBody;
    QString saveVectorTitle;
    QString saveVectorDefaultName;
    QString saveVectorFailedTitle;
    QString saveVectorFailedBody;
    QString exportVectorTitle;
    QString exportVectorBody;
    QString exportVectorStatus;
    QString restorePatternStatus;
    QString aboutWindowTitle;
    QString aboutBody;
    QString aboutRights;
    QString closeButton;
    QString engineStatusTitle;
    QString viewportCaption;
    QString sourceGroupTitle;
    QString loadImageButton;
    QString restorePatternButton;
    QString deflectionGroupTitle;
    QString hDeflectionToggle;
    QString vDeflectionToggle;
    QString timingGroupTitle;
    QString speedCaption;
    QString ioGroupTitle;
    QString exportVectorButton;
    QString loadVectorButton;
    QString fileMenuTitle;
    QString helpMenuTitle;
    QString languageMenuTitle;
    QString exitAction;
    QString quickAccessTitle;
    QString loadImageAction;
    QString loadVectorAction;
    QString exportVectorAction;
    QString restorePatternAction;
    QString aboutAction;
    QString rasterModeChip;
    QString vectorModeChip;
    QString rasterModeTelemetry;
    QString vectorModeTelemetry;
    QString sourceRasterFormat;
    QString sourceVectorFormat;
    QString defaultPatternName;
    QString pulseRateStatusFormat;
    QString telemetryTitle;
    QString telemetrySubtitle;
    QString telemetryModeKey;
    QString telemetryHKey;
    QString telemetryVKey;
    QString telemetryRKey;
    QString telemetryGKey;
    QString telemetryBKey;
    QString telemetryCmdsKey;
    QString rendererHint;
    QString languageArabic;
    QString languageEnglish;
    QString languageFrench;
    QString languageChinese;
};

inline QLocale localeForLanguage(AppLanguage language) {
    switch (language) {
    case AppLanguage::Arabic:
        return QLocale(QLocale::Arabic, QLocale::Egypt);
    case AppLanguage::French:
        return QLocale(QLocale::French, QLocale::France);
    case AppLanguage::Chinese:
        return QLocale(QLocale::Chinese, QLocale::China);
    case AppLanguage::English:
    default:
        return QLocale(QLocale::English, QLocale::UnitedStates);
    }
}

inline Qt::LayoutDirection layoutDirectionForLanguage(AppLanguage language) {
    return language == AppLanguage::Arabic ? Qt::RightToLeft : Qt::LeftToRight;
}

inline AppLanguage defaultLanguageFromSystem() {
    switch (QLocale::system().language()) {
    case QLocale::Arabic:
        return AppLanguage::Arabic;
    case QLocale::French:
        return AppLanguage::French;
    case QLocale::Chinese:
        return AppLanguage::Chinese;
    default:
        return AppLanguage::English;
    }
}

inline QString nativeLanguageName(AppLanguage language) {
    switch (language) {
    case AppLanguage::Arabic:
        return QStringLiteral("العربية");
    case AppLanguage::French:
        return QStringLiteral("Français");
    case AppLanguage::Chinese:
        return QStringLiteral("中文");
    case AppLanguage::English:
    default:
        return QStringLiteral("English");
    }
}

inline UiText uiText(AppLanguage language) {
    switch (language) {
    case AppLanguage::Arabic:
        return {
            QStringLiteral("محاكي شعاع CRT"),
            QStringLiteral("جاهز للمسح والتحليل."),
            QStringLiteral("اختر صورة للتحليل"),
            QStringLiteral("ملفات الصور (*.png *.jpg *.jpeg *.bmp *.gif);;كل الملفات (*.*)"),
            QStringLiteral("فشل التحميل"),
            QStringLiteral("تعذر قراءة الصورة أو تحويلها إلى دقة التحليل الداخلية."),
            QStringLiteral("تم تحميل الصورة في وضع التحليل النقطي."),
            QStringLiteral("اختر ملف أوامر متجهة"),
            QStringLiteral("ملفات JSON (*.json);;كل الملفات (*.*)"),
            QStringLiteral("ملف غير صالح"),
            QStringLiteral("الملف لا يحتوي على أوامر توجيه صالحة أو لا يمكن قراءته."),
            QStringLiteral("تم تحميل برنامج المتجهات"),
            QStringLiteral("تم تحميل أوامر المتجهات بنجاح.\nعدد الأوامر الفعالة: %1"),
            QStringLiteral("حفظ ملف المتجهات"),
            QStringLiteral("Vector_Gun_Commands.json"),
            QStringLiteral("تعذر الحفظ"),
            QStringLiteral("فشل إنشاء ملف إشارات المتجهات."),
            QStringLiteral("تم تصدير الإشارات"),
            QStringLiteral("تم حفظ الإشارات بنجاح.\nالأوامر النشطة: %1\nالأوامر التي تم تجاوزها: %2\nنسبة الضغط: %3%"),
            QStringLiteral("تم حفظ ملف المتجهات بنجاح."),
            QStringLiteral("تم استرجاع النمط الاختباري الافتراضي."),
            QStringLiteral("حول البرنامج"),
            QStringLiteral("محاكاة مكتبية لشعاع CRT والفوسفور وتشغيل المسارات النقطية والمتجهة في عرض واحد."),
            QStringLiteral("جميع الحقوق محفوظة. MechaML Researchers"),
            QStringLiteral("إغلاق"),
            QStringLiteral("حالة المحرك"),
            QStringLiteral("حجرة أنبوب CRT / العرض التشخيصي التفاعلي"),
            QStringLiteral("مصدر الإشارة"),
            QStringLiteral("تحميل صورة للتحليل"),
            QStringLiteral("استرجاع النمط الافتراضي"),
            QStringLiteral("ملفات الانحراف"),
            QStringLiteral("الانحراف الأفقي"),
            QStringLiteral("الانحراف الرأسي"),
            QStringLiteral("سرعة المعالجة"),
            QStringLiteral("معدل النبض"),
            QStringLiteral("إدخال وإخراج المتجهات"),
            QStringLiteral("حفظ ملف المتجهات"),
            QStringLiteral("تحميل ملف المتجهات"),
            QStringLiteral("ملف"),
            QStringLiteral("مساعدة"),
            QStringLiteral("اللغة"),
            QStringLiteral("خروج"),
            QStringLiteral("وصول سريع"),
            QStringLiteral("تحميل صورة للتحليل..."),
            QStringLiteral("تحميل ملف المتجهات..."),
            QStringLiteral("حفظ ملف المتجهات..."),
            QStringLiteral("استرجاع النمط الافتراضي"),
            QStringLiteral("حول البرنامج"),
            QStringLiteral("وضع المسح النقطي"),
            QStringLiteral("محرك الرسم المتجهي"),
            QStringLiteral("وضع متجهي"),
            QStringLiteral("وضع نقطي"),
            QStringLiteral("مصدر نقطي: %1"),
            QStringLiteral("برنامج متجهي: %1"),
            QStringLiteral("النمط الاختباري الداخلي"),
            QStringLiteral("تم ضبط معدل النبض على %1"),
            QStringLiteral("القياسات المباشرة"),
            QStringLiteral("قراءة فورية لمسار الشعاع وقدرة المدافع."),
            QStringLiteral("الوضع"),
            QStringLiteral("الانحراف الأفقي"),
            QStringLiteral("الانحراف الرأسي"),
            QStringLiteral("قدرة المدفع الأحمر"),
            QStringLiteral("قدرة المدفع الأخضر"),
            QStringLiteral("قدرة المدفع الأزرق"),
            QStringLiteral("الأوامر النشطة"),
            QStringLiteral("زر الفأرة الأيسر: تدوير العرض   |   العجلة: تقريب / إبعاد"),
            QStringLiteral("العربية"),
            QStringLiteral("English"),
            QStringLiteral("Français"),
            QStringLiteral("中文")
        };
    case AppLanguage::French:
        return {
            QStringLiteral("Simulateur de faisceau CRT"),
            QStringLiteral("Prêt pour le balayage et l'analyse."),
            QStringLiteral("Choisir une image à analyser"),
            QStringLiteral("Images (*.png *.jpg *.jpeg *.bmp *.gif);;Tous les fichiers (*.*)"),
            QStringLiteral("Échec du chargement"),
            QStringLiteral("Impossible de lire l'image ou de la convertir vers la résolution interne d'analyse."),
            QStringLiteral("Image chargée en mode raster."),
            QStringLiteral("Choisir un fichier de commandes vectorielles"),
            QStringLiteral("Fichiers JSON (*.json);;Tous les fichiers (*.*)"),
            QStringLiteral("Fichier invalide"),
            QStringLiteral("Le fichier ne contient pas de commandes vectorielles valides ou ne peut pas être lu."),
            QStringLiteral("Programme vectoriel chargé"),
            QStringLiteral("Les commandes vectorielles ont été chargées avec succès.\nCommandes actives : %1"),
            QStringLiteral("Enregistrer le fichier vectoriel"),
            QStringLiteral("Vector_Gun_Commands.json"),
            QStringLiteral("Enregistrement impossible"),
            QStringLiteral("Impossible de créer le fichier de signaux vectoriels."),
            QStringLiteral("Signaux exportés"),
            QStringLiteral("Les signaux ont été enregistrés avec succès.\nCommandes actives : %1\nCommandes ignorées : %2\nTaux de compression : %3 %"),
            QStringLiteral("Le fichier vectoriel a été enregistré."),
            QStringLiteral("Le motif de test par défaut a été restauré."),
            QStringLiteral("À propos"),
            QStringLiteral("Émulateur de bureau du faisceau CRT, du phosphore et de la lecture raster/vectorielle dans une vue unifiée."),
            QStringLiteral("Tous droits réservés. MechaML Researchers"),
            QStringLiteral("Fermer"),
            QStringLiteral("État du moteur"),
            QStringLiteral("Tube CRT / vue diagnostique interactive"),
            QStringLiteral("Source du signal"),
            QStringLiteral("Charger une image"),
            QStringLiteral("Restaurer le motif par défaut"),
            QStringLiteral("Déflexion"),
            QStringLiteral("Déflexion horizontale"),
            QStringLiteral("Déflexion verticale"),
            QStringLiteral("Vitesse de traitement"),
            QStringLiteral("Cadence d'impulsion"),
            QStringLiteral("Entrée / sortie vectorielle"),
            QStringLiteral("Enregistrer le vectoriel"),
            QStringLiteral("Charger le vectoriel"),
            QStringLiteral("Fichier"),
            QStringLiteral("Aide"),
            QStringLiteral("Langue"),
            QStringLiteral("Quitter"),
            QStringLiteral("Accès rapide"),
            QStringLiteral("Charger une image..."),
            QStringLiteral("Charger un fichier vectoriel..."),
            QStringLiteral("Enregistrer un fichier vectoriel..."),
            QStringLiteral("Restaurer le motif par défaut"),
            QStringLiteral("À propos"),
            QStringLiteral("Mode raster"),
            QStringLiteral("Moteur vectoriel"),
            QStringLiteral("Mode vectoriel"),
            QStringLiteral("Mode raster"),
            QStringLiteral("Source raster : %1"),
            QStringLiteral("Programme vectoriel : %1"),
            QStringLiteral("Motif de test interne"),
            QStringLiteral("Cadence d'impulsion réglée sur %1"),
            QStringLiteral("Télémétrie en direct"),
            QStringLiteral("Lecture instantanée de la trajectoire du faisceau et de la puissance des canons."),
            QStringLiteral("Mode"),
            QStringLiteral("Déflexion H"),
            QStringLiteral("Déflexion V"),
            QStringLiteral("Puissance canon R"),
            QStringLiteral("Puissance canon G"),
            QStringLiteral("Puissance canon B"),
            QStringLiteral("Commandes actives"),
            QStringLiteral("Bouton gauche : orbite   |   Molette : zoom"),
            QStringLiteral("العربية"),
            QStringLiteral("English"),
            QStringLiteral("Français"),
            QStringLiteral("中文")
        };
    case AppLanguage::Chinese:
        return {
            QStringLiteral("CRT 电子束模拟器"),
            QStringLiteral("已准备好进行扫描和分析。"),
            QStringLiteral("选择要分析的图像"),
            QStringLiteral("图像文件 (*.png *.jpg *.jpeg *.bmp *.gif);;所有文件 (*.*)"),
            QStringLiteral("加载失败"),
            QStringLiteral("无法读取图像，或无法转换为内部分析分辨率。"),
            QStringLiteral("图像已以光栅模式加载。"),
            QStringLiteral("选择矢量指令文件"),
            QStringLiteral("JSON 文件 (*.json);;所有文件 (*.*)"),
            QStringLiteral("无效文件"),
            QStringLiteral("文件不包含有效的矢量指令，或无法读取。"),
            QStringLiteral("矢量程序已加载"),
            QStringLiteral("矢量指令已成功加载。\n有效指令数：%1"),
            QStringLiteral("保存矢量文件"),
            QStringLiteral("Vector_Gun_Commands.json"),
            QStringLiteral("无法保存"),
            QStringLiteral("无法创建矢量信号文件。"),
            QStringLiteral("信号已导出"),
            QStringLiteral("信号已成功保存。\n活动指令：%1\n已跳过指令：%2\n压缩率：%3%"),
            QStringLiteral("矢量文件已成功保存。"),
            QStringLiteral("默认测试图案已恢复。"),
            QStringLiteral("关于"),
            QStringLiteral("桌面 CRT 电子束、荧光体以及光栅 / 矢量回放统一仿真器。"),
            QStringLiteral("保留所有权利。MechaML Researchers"),
            QStringLiteral("关闭"),
            QStringLiteral("引擎状态"),
            QStringLiteral("CRT 管腔 / 交互式诊断视图"),
            QStringLiteral("信号源"),
            QStringLiteral("加载分析图像"),
            QStringLiteral("恢复默认图案"),
            QStringLiteral("偏转"),
            QStringLiteral("水平偏转"),
            QStringLiteral("垂直偏转"),
            QStringLiteral("处理速度"),
            QStringLiteral("脉冲速率"),
            QStringLiteral("矢量输入 / 输出"),
            QStringLiteral("保存矢量文件"),
            QStringLiteral("加载矢量文件"),
            QStringLiteral("文件"),
            QStringLiteral("帮助"),
            QStringLiteral("语言"),
            QStringLiteral("退出"),
            QStringLiteral("快速操作"),
            QStringLiteral("加载分析图像..."),
            QStringLiteral("加载矢量文件..."),
            QStringLiteral("保存矢量文件..."),
            QStringLiteral("恢复默认图案"),
            QStringLiteral("关于"),
            QStringLiteral("光栅模式"),
            QStringLiteral("矢量绘制引擎"),
            QStringLiteral("矢量模式"),
            QStringLiteral("光栅模式"),
            QStringLiteral("光栅源：%1"),
            QStringLiteral("矢量程序：%1"),
            QStringLiteral("内置测试图案"),
            QStringLiteral("脉冲速率已设置为 %1"),
            QStringLiteral("实时遥测"),
            QStringLiteral("实时显示束流路径和电子枪功率。"),
            QStringLiteral("模式"),
            QStringLiteral("水平偏转"),
            QStringLiteral("垂直偏转"),
            QStringLiteral("红枪功率"),
            QStringLiteral("绿枪功率"),
            QStringLiteral("蓝枪功率"),
            QStringLiteral("活动指令"),
            QStringLiteral("左键：旋转视图   |   滚轮：缩放"),
            QStringLiteral("العربية"),
            QStringLiteral("English"),
            QStringLiteral("Français"),
            QStringLiteral("中文")
        };
    case AppLanguage::English:
    default:
        return {
            QStringLiteral("CRT Beam Simulator"),
            QStringLiteral("Ready for scan and analysis."),
            QStringLiteral("Choose an image to analyze"),
            QStringLiteral("Image Files (*.png *.jpg *.jpeg *.bmp *.gif);;All Files (*.*)"),
            QStringLiteral("Load Failed"),
            QStringLiteral("The image could not be read or converted to the internal analysis resolution."),
            QStringLiteral("Image loaded in raster analysis mode."),
            QStringLiteral("Choose a vector command file"),
            QStringLiteral("JSON Files (*.json);;All Files (*.*)"),
            QStringLiteral("Invalid File"),
            QStringLiteral("The file does not contain valid vector commands or could not be read."),
            QStringLiteral("Vector Program Loaded"),
            QStringLiteral("Vector commands loaded successfully.\nActive commands: %1"),
            QStringLiteral("Save vector file"),
            QStringLiteral("Vector_Gun_Commands.json"),
            QStringLiteral("Save Failed"),
            QStringLiteral("The vector signal file could not be created."),
            QStringLiteral("Signals Exported"),
            QStringLiteral("Signals saved successfully.\nActive commands: %1\nSkipped commands: %2\nCompression ratio: %3%"),
            QStringLiteral("Vector file saved successfully."),
            QStringLiteral("Default test pattern restored."),
            QStringLiteral("About"),
            QStringLiteral("Desktop emulator for CRT beam, phosphor response, and raster / vector playback in a single view."),
            QStringLiteral("All rights reserved. MechaML Researchers"),
            QStringLiteral("Close"),
            QStringLiteral("Engine Status"),
            QStringLiteral("CRT Tube Chamber / Interactive Diagnostic View"),
            QStringLiteral("Signal Source"),
            QStringLiteral("Load Analysis Image"),
            QStringLiteral("Restore Default Pattern"),
            QStringLiteral("Deflection"),
            QStringLiteral("Horizontal Deflection"),
            QStringLiteral("Vertical Deflection"),
            QStringLiteral("Processing Speed"),
            QStringLiteral("Pulse Rate"),
            QStringLiteral("Vector I/O"),
            QStringLiteral("Save Vector File"),
            QStringLiteral("Load Vector File"),
            QStringLiteral("File"),
            QStringLiteral("Help"),
            QStringLiteral("Language"),
            QStringLiteral("Exit"),
            QStringLiteral("Quick Access"),
            QStringLiteral("Load Analysis Image..."),
            QStringLiteral("Load Vector File..."),
            QStringLiteral("Save Vector File..."),
            QStringLiteral("Restore Default Pattern"),
            QStringLiteral("About"),
            QStringLiteral("Raster Mode"),
            QStringLiteral("Vector Draw Engine"),
            QStringLiteral("Vector Mode"),
            QStringLiteral("Raster Mode"),
            QStringLiteral("Raster Source: %1"),
            QStringLiteral("Vector Program: %1"),
            QStringLiteral("Internal Test Pattern"),
            QStringLiteral("Pulse rate set to %1"),
            QStringLiteral("Live Telemetry"),
            QStringLiteral("Instant readout for beam path and gun drive levels."),
            QStringLiteral("Mode"),
            QStringLiteral("H Deflection"),
            QStringLiteral("V Deflection"),
            QStringLiteral("R Gun Power"),
            QStringLiteral("G Gun Power"),
            QStringLiteral("B Gun Power"),
            QStringLiteral("Active Commands"),
            QStringLiteral("LMB: Orbit view   |   Wheel: Zoom"),
            QStringLiteral("العربية"),
            QStringLiteral("English"),
            QStringLiteral("Français"),
            QStringLiteral("中文")
        };
    }
}
