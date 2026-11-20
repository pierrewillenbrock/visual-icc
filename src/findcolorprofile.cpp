
#include <QCoreApplication>
#include <QDir>
#include <QPluginLoader>
#include <QJsonArray>
#include "colorprofileproviders/colorprofileprovider.h"

// under x11:
//   the x11 loader will be able to reliably provide access to the icc profile, if any, so it has a high priority.
//   the kde plugin may not be able to provide icc _or_ info if it is using edid instead.
// under wayland:
//   the kde plugin will be able to tell us if icc or edid is in use and then provide filename.

struct FindColorProfileInformationCandidate {
    int prio;///< Priority, higher means this is used in favor of lowers
    QString path;///< Path to the library
    ColorProfileProvider *provider; ///< The actual provider object
    QPluginLoader *loader;
};

/// List of color profile providers, also keeping references to the actual objects
static QList<FindColorProfileInformationCandidate> findColorProfileInformationList;

ColorProfileInformation findColorProfileInformation(const QString &output) {
    if(findColorProfileInformationList.isEmpty()) {
        const QString desktops = qEnvironmentVariable("XDG_CURRENT_DESKTOP");

        QStringList dirs = { QCoreApplication::applicationDirPath() + "/colorprofile" };
        for(const QString &lp : QCoreApplication::libraryPaths()) {
            dirs << lp + "/colorprofile";
        }
        for(const QString &dir : dirs) {
            //qDebug() << "Looking in dir " << dir;
            for(const QString &file : QDir(dir).entryList({"*.so"}, QDir::Files)) {
                const QString path = QDir(dir).filePath(file);

                auto *loader = new QPluginLoader(path);

                const QJsonObject meta = loader->metaData().value("MetaData").toObject();
                const QJsonArray want = meta.value("desktops").toArray();
                bool match = want.isEmpty();
                for(const auto &d : want) {
                    if(desktops.contains(d.toString(), Qt::CaseInsensitive)) {
                        match = true;
                    }
                }
                if(match) {
                    findColorProfileInformationList.append({meta.value("priority").toInt(), path, nullptr, loader});
                }
            }
        }
        std::sort(findColorProfileInformationList.begin(),
        findColorProfileInformationList.end(), [](auto &a, auto &b) {
            return a.prio > b.prio;
        });
    }

    for (auto &c : findColorProfileInformationList) {
        //we do not unload the library and keep any object we found.
        if(!c.provider && c.loader) {
            auto *obj = c.loader->instance();
            auto *prov = qobject_cast<ColorProfileProvider *>(obj);
            if(!prov) {
                qDebug() << c.path << c.loader->errorString();
                c.loader->unload();
                delete c.loader;
                c.loader = nullptr;
                continue;
            }
            c.provider = prov;
        }
        if(!c.provider->isAvailable()) {
            continue;
        }
        ColorProfileInformation info = c.provider->profileInformation(output);
        if(info) {
            return info;
        }
    }
    return {};
}

