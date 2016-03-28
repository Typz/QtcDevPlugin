#include "qtcrunconfiguration.h"

#include "qtcdevpluginconstants.h"
#include "Widgets/filetypevalidatinglineedit.h"

#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/target.h>
#include <projectexplorer/kit.h>

#include <qtsupport/qtsupportconstants.h>

#include <qmakeprojectmanager/qmakeproject.h>

#include <coreplugin/variablechooser.h>
#include <coreplugin/icore.h>

#include <utils/theme/theme.h>

#include <QtWidgets>
#include <QtDebug>

namespace QtcDevPlugin {
namespace Internal {

QStringList availableThemes(void)
{
    QStringList themes;

    QDir resourceDir = QDir(Core::ICore::resourcePath());
    resourceDir.cd(QLatin1String("themes"));
    resourceDir.setFilter(QDir::Files);
    resourceDir.setNameFilters(QStringList() << QLatin1String("*.creatortheme"));
    foreach (QFileInfo info, resourceDir.entryInfoList()) {
        themes << info.completeBaseName();
    }

    int defaultIndex = themes.indexOf(QLatin1String("default"));
    if (defaultIndex != -1)
        themes.prepend(themes.takeAt(defaultIndex));
    else
        qWarning() << "\"default\" theme not found in ressource path.";

    QDir userResourceDir = QDir(Core::ICore::userResourcePath());
    userResourceDir.cd(QLatin1String("themes"));
    userResourceDir.setFilter(QDir::Files);
    userResourceDir.setNameFilters(QStringList() << QLatin1String("*.creatortheme"));
    foreach (QFileInfo info, userResourceDir.entryInfoList()) {
        themes << info.completeBaseName();
    }

    qDebug() << themes << resourceDir.absolutePath() << userResourceDir.absolutePath();

    return themes;
}

QtcRunConfiguration::QtcRunConfiguration(ProjectExplorer::Target *parent, Core::Id id):
    ProjectExplorer::LocalApplicationRunConfiguration(parent, id)
{
    mWorkingDirectory = Utils::FileName::fromString(QLatin1String("%{buildDir}"));
    mThemeName = Utils::creatorTheme()->displayName();

    setDefaultDisplayName(tr("Run Qt Creator"));

    /* TODO ensure this run configuration cannot be run with valgrind...
     * To do this, the code of the Valgrind plugin should be altered:
     * 1.ValgrindRunControlFactory should check the type of the given RunConfiguration (e.g. in canRun())
     * and addAspects() should only add aspects provided bu runnable RunControl factories.
     * 2.Alternatively, ValgrindPlugin, should ensure the extra aspects are added to
     * sensible RunConfiguration and RunConfiguration::addExtraAspects() should be removed. */
    addExtraAspect(new ProjectExplorer::LocalEnvironmentAspect(this));
}

QVariantMap QtcRunConfiguration::toMap(void) const
{
    QVariantMap map(ProjectExplorer::RunConfiguration::toMap());
    if (QString::compare(mWorkingDirectory.toString(), QLatin1String("%{buildDir}"), Utils::HostOsInfo::fileNameCaseSensitivity()) != 0)
        map.insert(Constants::WorkingDirectoryId, mWorkingDirectory.toString());
    if (!mSettingsPath.isNull())
        map.insert(Constants::SettingsPathId, mSettingsPath.toString());

    return map;
}

bool QtcRunConfiguration::fromMap(const QVariantMap& map)
{
    mWorkingDirectory = Utils::FileName::fromString(map.value(Constants::WorkingDirectoryId, QLatin1String("%{buildDir}")).toString());
    mSettingsPath = Utils::FileName::fromString(map.value(Constants::SettingsPathId, QString()).toString());

    return ProjectExplorer::RunConfiguration::fromMap(map);
}

QString QtcRunConfiguration::workingDirectory(void) const
{
    if (macroExpander() != NULL)
        return macroExpander()->expand(mWorkingDirectory.toString());
    return mWorkingDirectory.toString();
}

QStringList QtcRunConfiguration::commandLineArgumentsList(void) const
{
    QStringList cmdArgs;

    cmdArgs << QLatin1String("-theme") << mThemeName;

    QString pluginsPath = mDestDir.toString();
    pluginsPath.replace(QLatin1Char('"'), QLatin1String("\\\""));
    if (pluginsPath.contains(QLatin1Char(' ')))
        pluginsPath.prepend(QLatin1Char('"')).append(QLatin1Char('"'));
    cmdArgs << QLatin1String("-pluginpath") << pluginsPath;

    QString settingsPath = mSettingsPath.toString();
    if (macroExpander() != NULL)
        settingsPath = macroExpander()->expand(settingsPath);
    settingsPath.replace(QLatin1Char('"'), QLatin1String("\\\""));
    if (settingsPath.contains(QLatin1Char(' ')))
        settingsPath.prepend(QLatin1Char('"')).append(QLatin1Char('"'));
    if (!mSettingsPath.isNull())
        cmdArgs << QLatin1String("-settingspath") << settingsPath;

    qDebug() << "Run config command line arguments:" << cmdArgs;
    return cmdArgs;
}

QtcRunConfigurationWidget::QtcRunConfigurationWidget(QtcRunConfiguration* runConfig, QWidget* parent)
    : QWidget(parent), mRunConfig(runConfig)
{
    mWorkingDirectoryEdit = new Widgets::FileTypeValidatingLineEdit(this);
    mWorkingDirectoryEdit->setMacroExpander(runConfig->macroExpander());
    mWorkingDirectoryEdit->setAcceptDirectories(true);
    mWorkingDirectoryLabel = new QLabel(tr("Working directory:"), this);
    mWorkingDirectoryLabel->setBuddy(mWorkingDirectoryEdit);
    mWorkingDirectoryButton = new QPushButton(tr("Browse..."), this);

    mSettingsPathEdit = new Widgets::FileTypeValidatingLineEdit(this);
    mSettingsPathEdit->setMacroExpander(runConfig->macroExpander());
    mSettingsPathEdit->setAcceptDirectories(true);
    mSettingsPathEdit->setEnabled(false);
    mSettingsPathCheck = new QCheckBox(tr("Use alternative settings path"));
    mSettingsPathCheck->setChecked(false);
    mSettingsPathButton = new QPushButton(tr("Browse..."), this);
    mSettingsPathButton->setEnabled(false);

    mThemeCombo = new QComboBox(this);
    mThemeCombo->setEditable(false);
    mThemeLabel = new QLabel(tr("Theme:"), this);
    mThemeLabel->setBuddy(mThemeCombo);

    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(mWorkingDirectoryLabel, 0, 0, Qt::AlignLeft);
    mainLayout->addWidget(mWorkingDirectoryEdit, 0, 1);
    mainLayout->addWidget(mWorkingDirectoryButton, 0, 2, Qt::AlignCenter);
    mainLayout->addWidget(mSettingsPathCheck, 1, 0, Qt::AlignLeft);
    mainLayout->addWidget(mSettingsPathEdit, 1, 1);
    mainLayout->addWidget(mSettingsPathButton, 1, 2, Qt::AlignCenter);
    mainLayout->addWidget(mThemeLabel, 2, 0, Qt::AlignLeft);
    mainLayout->addWidget(mThemeCombo, 2, 1, 1, 2, Qt::AlignRight);
    mainLayout->setColumnStretch(1, 1);

    QStringList themes = availableThemes();
    int currentThemeIndex = themes.indexOf(runConfig->mThemeName);
    QTC_ASSERT(currentThemeIndex != -1, );
    mThemeCombo->addItems(themes);
    if (currentThemeIndex != -1)
        mThemeCombo->setCurrentIndex(currentThemeIndex);

    Core::VariableChooser::addSupportForChildWidgets(this, runConfig->macroExpander());

    setLayout(mainLayout);

    connect(mWorkingDirectoryEdit, SIGNAL(validChanged(bool)),
            this, SLOT(updateWorkingDirectory(bool)));
    connect(mWorkingDirectoryEdit, SIGNAL(editingFinished()),
            this, SLOT(updateWorkingDirectory()));
    connect(mWorkingDirectoryButton, SIGNAL(released()),
            this, SLOT(browseWorkingDirectory()));

    connect(mSettingsPathCheck, SIGNAL(toggled(bool)),
            this, SLOT(updateSettingsPathState(bool)));
    connect(mSettingsPathEdit, SIGNAL(validChanged(bool)),
            this, SLOT(updateSettingsPath(bool)));
    connect(mSettingsPathEdit, SIGNAL(editingFinished()),
            this, SLOT(updateSettingsPath()));
    connect(mSettingsPathButton, SIGNAL(released()),
            this, SLOT(browseSettingsPath()));

    connect(mThemeCombo, SIGNAL(currentIndexChanged(const QString&)),
            this, SLOT(updateTheme(const QString&)));
}

void QtcRunConfigurationWidget::showEvent(QShowEvent *se)
{
    mWorkingDirectoryEdit->setText(mRunConfig->mWorkingDirectory.toString());

    QWidget::showEvent(se);
}

void QtcRunConfigurationWidget::updateWorkingDirectory(bool valid)
{
    if (valid)
        mRunConfig->mWorkingDirectory = Utils::FileName::fromUserInput(mWorkingDirectoryEdit->text());
}

void QtcRunConfigurationWidget::updateWorkingDirectory(void)
{
    if (mWorkingDirectoryEdit->isValid())
        mRunConfig->mWorkingDirectory = Utils::FileName::fromUserInput(mWorkingDirectoryEdit->text());
    mWorkingDirectoryEdit->setText(mRunConfig->mWorkingDirectory.toString());
}

void QtcRunConfigurationWidget::browseWorkingDirectory(void)
{
    QString wd = QFileDialog::getExistingDirectory(this, tr("Choose working directory"), mRunConfig->mWorkingDirectory.toString());

    if (!wd.isNull())
        mRunConfig->mWorkingDirectory = Utils::FileName::fromString(wd);
    mWorkingDirectoryEdit->setText(mRunConfig->mWorkingDirectory.toString());
}

void QtcRunConfigurationWidget::updateSettingsPathState(bool checked)
{
    mSettingsPathEdit->setEnabled(checked);
    mSettingsPathButton->setEnabled(checked);
    mRunConfig->mSettingsPath = checked ? Utils::FileName::fromUserInput(mSettingsPathEdit->text()) : Utils::FileName();
}

void QtcRunConfigurationWidget::updateSettingsPath(bool valid)
{
    if (valid)
        mRunConfig->mSettingsPath = Utils::FileName::fromUserInput(mSettingsPathEdit->text());
}

void QtcRunConfigurationWidget::updateSettingsPath(void)
{
    if (mSettingsPathEdit->isValid())
        mRunConfig->mSettingsPath = Utils::FileName::fromUserInput(mSettingsPathEdit->text());
    mSettingsPathEdit->setText(mRunConfig->mSettingsPath.toString());
}

void QtcRunConfigurationWidget::browseSettingsPath(void)
{
    QString wd = QFileDialog::getExistingDirectory(this, tr("Choose alternative settings path"), mRunConfig->mSettingsPath.toString());

    if (!wd.isNull())
        mRunConfig->mSettingsPath = Utils::FileName::fromString(wd);
    mSettingsPathEdit->setText(mRunConfig->mSettingsPath.toString());
}

void QtcRunConfigurationWidget::updateTheme(const QString& theme)
{
    mRunConfig->mThemeName = theme;
}

} // Internal
} // QTestLibPlugin
