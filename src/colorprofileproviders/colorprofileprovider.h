#pragma once

#include <QByteArray>
#include <QString>
#include <QtPlugin>
#include <QColorSpace>

#include <optional>

/** Color profile information structure
 *
 * Contains information about a displays color profile
 */
struct ColorProfileInformation {
    QString output; ///< output name
    enum {
        CPIT_NONE, ///< the information could not be gathered
        CPIT_EDID, ///< The information has been derived from EDID information
        CPIT_ICC  ///< The information has been derived from an ICC profile, possibly a file
    } type = CPIT_NONE; ///< Type of information
    QColorSpace colorSpace; ///< colorspace of the output display
    /** icc profile file if any.
     *
     * This might be empty even though the data is available.
     * There is no raw icc data here, the user can gather that themselves if they want it.
     */
    QString iccProfileFile;
    /// name of the provider, e.g. the plugin name
    QString providerPlugin;
    /// @return true if valid
    operator bool() { return type != CPIT_NONE; }
    ColorProfileInformation() =default;
};

/** ColorProfile provider base class
 */
class ColorProfileProvider
{
public:
    virtual ~ColorProfileProvider() = default;

    /// cheap way to check if we have a chance to do this.
    virtual bool isAvailable() const = 0;

    /** Collect profile information
     *
     * @param output Connector-Name ("DP-1"), if empty, look for the primary output
     * @return the information. evaluates false if none could be gathered.
     */
    virtual ColorProfileInformation profileInformation(const QString &output = {}) const =0;
};

#define ColorProfileProvider_iid "de.pirsoft.ColorProfileProvider/1"
Q_DECLARE_INTERFACE(ColorProfileProvider, ColorProfileProvider_iid)

