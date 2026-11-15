// ObjC++-only TICueCard conversion for the other Bridge/ wrappers. Never
// import from the Swift bridging header.
#import "TIFileFormats.h"
#include <calcu59/app/CueCard.hpp>
#include <calcu59/app/ModuleCatalog.hpp>

/// TICueCard ↔ cu59::CueCardContent.
TICueCard* TICueCardFrom(const cu59::CueCardContent& c);
cu59::CueCardContent CueCardFrom(TICueCard* t);
/// cu59::ModuleMetadata → TIModuleMetadata.
TIModuleMetadata* TIModuleMetadataFrom(const cu59::ModuleMetadata& m);
