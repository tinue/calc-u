// ObjC++ only: what TIAppSession and TIShell tell each other.
// Never import from the Swift bridging header.
#import "TIAppSession.h"
#import "TIShell.h"
#include "sessions/Messages.hpp"
#include "shell/Inputs.hpp"
#include "shell/ShellProduct.hpp"
#include <functional>
#include <string>
#include <vector>

@interface TIShell ()
- (instancetype)initWithSession:(TIAppSession *)session;
/// A notice of the `notices` topic.
- (void)noticeArrived:(const std::string &)sentence;
/// The busy topic changed ("Loading…").
- (void)busyChanged;
/// The machine changed model: every pointer lets go.
- (void)modelChanged;
/// The product's shell data: the session follows the topics its items
/// follow (shell::followProductTopics).
+ (const calcu::shell::ShellProduct &)product;
@end

@interface TIAppSession ()
/// The last Value of each input topic, kept by the bridge's drain.
- (const calcu::shell::ShellInputs &)shellInputs;
- (void)setPresetList:(calcu::sessions::Value)list;
/// A command of the shell's, its reply on the main thread.
- (void)sendCommand:(calcu::sessions::Command)command
              reply:(std::function<void(const calcu::sessions::ReplyMessage &)>)reply;
/// A request of the shell's (the preset list), its reply on the main thread.
- (void)askRequest:(calcu::sessions::Request)request
             reply:(std::function<void(const calcu::sessions::ReplyMessage &)>)reply;
/// The delegate: the notice queue or the inputs changed.
- (void)shellNoticesChanged;
- (void)shellInputsChanged;
@end
