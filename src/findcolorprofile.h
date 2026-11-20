
#pragma once

#include "colorprofileproviders/colorprofileprovider.h"
#include <QString>

/** Collect information about the given display
 *
 * @param output The output to find information about. If left empty,
 *               uses either the primary output(if that concept exists) or
 *               the first output found.
 * @return Information about the output, see the @c ColorProfileInformation type
 */
ColorProfileInformation findColorProfileInformation(const QString &output = QString());
