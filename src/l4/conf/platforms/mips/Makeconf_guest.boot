# Customized bootstrap configuration file
#
# This is a make snippet.
#
# Copy this example file to Makeconf.boot in the same directory.
#
#
#
# Generic options:
#
# Search path for modules, such as binaries, libraries, kernel, configuration
# files or any other data file you might want to load. Note that the bin and
# lib directories of the build-tree are automatically added to the search
# path.
# MODULE_SEARCH_PATH = /path/to/cfgs:/path/to/foo:..

# set bootstrap build's search path for included modules
BOOTSTRAP_ELF_NAME     = guest_os
BOOTSTRAP_SEARCH_PATH  = $(L4DIR_ABS)/conf/examples
BOOTSTRAP_SEARCH_PATH += $(L4DIR_ABS)/../../build-guest/fiasco
BOOTSTRAP_SEARCH_PATH += $(L4DIR_ABS)/../../build/linux
BOOTSTRAP_SEARCH_PATH += $(L4DIR_ABS)/../../build/linux-simple
BOOTSTRAP_SEARCH_PATH += $(L4DIR_ABS)/pkg/karma/config
BOOTSTRAP_SEARCH_PATH += $(L4DIR_ABS)/pkg/karma/files
BOOTSTRAP_SEARCH_PATH += $(L4DIR_ABS)/pkg/io/config
BOOTSTRAP_SEARCH_PATH += $(L4DIR_ABS)/conf/platforms/mips
BOOTSTRAP_MODULES_LIST = $(L4DIR_ABS)/conf/platforms/mips/modules.list
