/** @file updatersettingsdialog.cpp Dialog for configuring automatic updates. 
 * @ingroup updater
 *
 * @authors Copyright © 2012-2013 Jaakko Keränen <jaakko.keranen@iki.fi>
 * @authors Copyright © 2013 Daniel Swanson <danij@dengine.net>
 *
 * @par License
 * GPL: http://www.gnu.org/licenses/gpl.html
 *
 * <small>This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version. This program is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details. You should have received a copy of the GNU
 * General Public License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA</small>
 */

#include "updatersettingsdialog.h"
#include "updatersettings.h"
#include <QDesktopServices>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <de/Log>
#include <QDebug>

using namespace de;

static QString defaultLocationName()
{
#ifdef DENG2_QT_5_0_OR_NEWER
    QString name = QStandardPaths::displayName(QStandardPaths::CacheLocation);
#else
    QString name = QDesktopServices::displayName(QDesktopServices::CacheLocation);
#endif
    if(name.isEmpty())
    {
        name = "Temporary Files";
    }
    return name;
}

DENG2_PIMPL(UpdaterSettingsDialog)
{
    QCheckBox* autoCheck;
    QComboBox* freqList;
    QLabel* lastChecked;
    QComboBox* channelList;
    QComboBox* pathList;
    QCheckBox* deleteAfter;

    Instance(Public &i) : Base(i)
    {
        // As a modal dialog it is implicitly clear that this belongs to
        // Doomsday, so we don't need to have the name in the window title.
        self.setWindowTitle(tr("Updater Settings"));

        QVBoxLayout* mainLayout = new QVBoxLayout;
        self.setLayout(mainLayout);

        QFormLayout* form = new QFormLayout;
        mainLayout->addLayout(form);

        freqList = new QComboBox;
        freqList->addItem(tr("At startup"), UpdaterSettings::AtStartup);
        freqList->addItem(tr("Daily"),      UpdaterSettings::Daily);
        freqList->addItem(tr("Biweekly"),   UpdaterSettings::Biweekly);
        freqList->addItem(tr("Weekly"),     UpdaterSettings::Weekly);
        freqList->addItem(tr("Monthly"),    UpdaterSettings::Monthly);

        autoCheck = new QCheckBox(tr("&Check for updates:"));
        form->addRow(autoCheck, freqList);
        QLayoutItem* item = form->itemAt(0, QFormLayout::LabelRole);
        item->setAlignment(Qt::AlignVCenter);

        lastChecked = new QLabel;
        form->addRow(new QWidget, lastChecked);

        channelList = new QComboBox;
        channelList->addItem(tr("Stable"), UpdaterSettings::Stable);
        channelList->addItem(tr("Unstable/Candidate"), UpdaterSettings::Unstable);
        form->addRow(tr("&Release type:"), channelList);

        pathList = new QComboBox;
        pathList->addItem(defaultLocationName(),
                          UpdaterSettings::defaultDownloadPath().toString());
        pathList->addItem(tr("Select folder..."), "");
        form->addRow(tr("&Download location:"), pathList);

        deleteAfter = new QCheckBox(tr("D&elete file after install"));
        form->addRow(new QWidget, deleteAfter);

        QDialogButtonBox* bbox = new QDialogButtonBox;
        mainLayout->addWidget(bbox);

        // Buttons.
        QPushButton* ok = bbox->addButton(QDialogButtonBox::Ok);
        QPushButton* cancel = bbox->addButton(QDialogButtonBox::Cancel);

        fetch();

        // Connections.
        QObject::connect(autoCheck, SIGNAL(toggled(bool)), &self, SLOT(autoCheckToggled(bool)));
        QObject::connect(pathList, SIGNAL(activated(int)), &self, SLOT(pathActivated(int)));
        QObject::connect(ok, SIGNAL(clicked()), &self, SLOT(accept()));
        QObject::connect(cancel, SIGNAL(clicked()), &self, SLOT(reject()));
    }

    void fetch()
    {
        UpdaterSettings st;

        String ago = st.lastCheckAgo();
        if(!ago.isEmpty())
        {
            lastChecked->setText(tr("<small>Last checked %1.</small>")
                                 .arg(st.lastCheckAgo()));
        }
        else
        {
            lastChecked->setText(tr("<small>Never checked.</small>"));
        }

        autoCheck->setChecked(!st.onlyCheckManually());
        freqList->setEnabled(!st.onlyCheckManually());
        freqList->setCurrentIndex(freqList->findData(st.frequency()));
        channelList->setCurrentIndex(channelList->findData(st.channel()));
        setDownloadPath(st.downloadPath());
        deleteAfter->setChecked(st.deleteAfterUpdate());
    }

    void apply()
    {
        UpdaterSettings st;
        st.setOnlyCheckManually(!autoCheck->isChecked());
        int sel = freqList->currentIndex();
        if(sel >= 0)
        {
            st.setFrequency(UpdaterSettings::Frequency(freqList->itemData(sel).toInt()));
        }
        sel = channelList->currentIndex();
        if(sel >= 0)
        {
            st.setChannel(UpdaterSettings::Channel(channelList->itemData(sel).toInt()));
        }
        st.setDownloadPath(pathList->itemData(pathList->currentIndex()).toString());
        st.setDeleteAfterUpdate(deleteAfter->isChecked());
    }

    void setDownloadPath(const QString& dir)
    {
        if(dir != UpdaterSettings::defaultDownloadPath())
        {
            // Remove extra items.
            while(pathList->count() > 2)
            {
                pathList->removeItem(0);
            }
            pathList->insertItem(0, QDir(dir).dirName(), dir);
            pathList->setCurrentIndex(0);
        }
        else
        {
            pathList->setCurrentIndex(pathList->findData(dir));
        }
    }
};

UpdaterSettingsDialog::UpdaterSettingsDialog(QWidget *parent)
    : UpdaterDialog(parent), d(new Instance(*this))
{}

void UpdaterSettingsDialog::fetch()
{
    d->fetch();
}

void UpdaterSettingsDialog::accept()
{
    d->apply();
    QDialog::accept();
}

void UpdaterSettingsDialog::reject()
{
    QDialog::reject();
}

void UpdaterSettingsDialog::autoCheckToggled(bool set)
{
    d->freqList->setEnabled(set);
}

void UpdaterSettingsDialog::pathActivated(int index)
{
    QString path = d->pathList->itemData(index).toString();
    if(path.isEmpty())
    {
        QString dir = QFileDialog::getExistingDirectory(this, tr("Download Folder"), QDir::homePath());
        if(!dir.isEmpty())
        {
            d->setDownloadPath(dir);
        }
        else
        {
            d->setDownloadPath(UpdaterSettings::defaultDownloadPath());
        }
    }
}
