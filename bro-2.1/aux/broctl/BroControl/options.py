# Configuration options. 
#
# If started directly, will print option reference documentation.

class Option:
    # Options category.
    USER = 1       # Standard user-configurable option.
    INTERNAL = 2   # internal, don't expose to user.
    AUTOMATIC = 3  # Set automatically, unlikely to be changed.

    def __init__(self, name, default, type, category, dontinit, description):
        self.name = name
        self.default = default
        self.type = type
        self.dontinit = dontinit
        self.category = category
        self.description = description

options = [
    # User options.
    Option("Debug", "0", "bool", Option.USER, False,
           "Enable extensive debugging output in spool/debug.log."),

    Option("HaveNFS", "0", "bool", Option.USER, False,
           "True if shared files are mounted across all nodes via NFS (see FAQ)."),
    Option("SaveTraces", "0", "bool", Option.USER, False,
           "True to let backends capture short-term traces via '-w'. These are not archived but might be helpful for debugging."),

    Option("StopTimeout", "60", "int", Option.USER, False,
           "The number of seconds to wait before sending a SIGKILL to a node which was previously issued the 'stop' command but did not terminate gracefully."),
    Option("CommTimeout", "10", "int", Option.USER, False,
           "The number of seconds to wait before assuming Broccoli communication events have timed out."),
    Option("LogRotationInterval", "3600", "int", Option.USER, False,
           "The frequency of log rotation in seconds for the manager/standalone node."),
    Option("LogDir", "${BroBase}/logs", "string", Option.USER, False,
           "Directory for archived log files."),
    Option("MakeArchiveName", "${BroBase}/share/broctl/scripts/make-archive-name", "string", Option.USER, False,
           "Script to generate filenames for archived log files."),
    Option("CompressLogs", "1", "bool", Option.USER, False,
           "True to gzip archived log files."),

    Option("SendMail", "@SENDMAIL@", "string", Option.USER, False,
           "Location of the sendmail binary.  Make this string blank to prevent email from being sent. The default value is configuration-dependent and determined automatically by CMake at configure-time."),
    Option("MailSubjectPrefix", "[Bro]", "string", Option.USER, False,
           "General Subject prefix for mails."),

    Option("MailReplyTo", "", "string", Option.USER, False,
           "Reply-to address for broctl-generated mails."),
    Option("MailTo", "<user>", "string", Option.USER, True,
           "Destination address for non-alarm mails."),
    Option("MailFrom", "Big Brother <bro@localhost>", "string", Option.USER, True,
           "Originator address for broctl-generated mails."),

    Option("MailAlarmsTo", "${MailTo}", "string", Option.USER, True,
           "Destination address for alarm summary mails. Default is to use the same address as MailTo."),

    Option("MinDiskSpace", "5", "int", Option.USER, False,
           "Percentage of minimum disk space available before warning is mailed."),
    Option("LogExpireInterval", "0", "int", Option.USER, False,
           "Number of days log files are kept (zero means disabled)."),
    Option("BroArgs", "", "string", Option.USER, False,
           "Additional arguments to pass to Bro on the command-line."),
    Option("MemLimit", "unlimited", "string", Option.USER, False,
           "Maximum amount of memory for Bro processes to use (in KB, or the string 'unlimited')."),

    Option("TimeFmt", "%d %b %H:%M:%S", "string", Option.USER, False,
           "Format string to print date/time specifications (see 'man strftime')."),

    Option("Prefixes", "local", "string", Option.USER, False,
           "Additional script prefixes for Bro, separated by colons. Use this instead of @prefix."),

    Option("SitePolicyManager", "local-manager.bro", "string", Option.USER, False,
           "Space-separated list of local policy files for manager."),
    Option("SitePolicyWorker", "local-worker.bro", "string", Option.USER, False,
           "Space-separated list of local policy files for workers."),
    Option("SitePolicyStandalone", "local.bro", "string", Option.USER, False,
           "Space-separated list of local policy files for all Bro instances."),

    Option("CronCmd", "", "string", Option.USER, False,
           "A custom command to run everytime the cron command has finished."),

    Option("PFRINGClusterID", "@PF_RING_CLUSTER_ID@", "int", Option.USER, False,
           "If PF_RING flow-based load balancing is desired, this is where the PF_RING cluster id is defined. The default value is configuration-dependent and determined automatically by CMake at configure-time based upon whether PF_RING's enhanced libpcap is available.  Bro must be linked with PF_RING's libpcap wrapper for this option to work."),

    Option("CFlowAddress", "", "string", Option.USER, False,
           "If a cFlow load-balancer is used, the address of the device (format: <ip>:<port>)."),
    Option("CFlowUser", "", "string", Option.USER, False,
           "If a cFlow load-balancer is used, the user name for accessing its configuration interface."),
    Option("CFlowPassword", "", "string", Option.USER, False,
           "If a cFlow load-balancer is used, the password for accessing its configuration interface."),

    Option("TimeMachineHost", "", "string", Option.USER, False,
           "If the manager should connect to a Time Machine, the address of the host it is running on."),
    Option("TimeMachinePort", "47757/tcp", "string", Option.USER, False,
           "If the manager should connect to a Time Machine, the port it is running on (in Bro syntax, e.g., 47757/tcp)."),

    Option("IPv6Comm", "1", "bool", Option.USER, False,
           "Enable IPv6 communication between cluster nodes (and also between them and BroControl)"),
    Option("ZoneID", "", "string", Option.USER, False,
           "If the host running BroControl is managing a cluster comprised of nodes with non-global IPv6 addresses, this option indicates what RFC 4007 zone_id to append to node addresses when communicating with them."),

    # Automatically set.
    Option("BroBase", "", "string", Option.AUTOMATIC, True,
           "Base path of broctl installation on all nodes."),
    Option("Version", "", "string", Option.AUTOMATIC, True,
           "Version of the broctl."),
    Option("StandAlone", "0", "bool", Option.AUTOMATIC, True,
           "True if running in stand-alone mode (see elsewhere)."),
    Option("OS", "", "string", Option.AUTOMATIC, True,
           "Name of operating system as reported by uname."),
    Option("Time", "", "string", Option.AUTOMATIC, True,
           "Path to time binary."),

    Option("BinDir", "${BroBase}/bin", "string", Option.AUTOMATIC, False,
           "Directory for executable files."),
    Option("ScriptsDir", "${BroBase}/share/broctl/scripts", "string", Option.AUTOMATIC, False,
           "Directory for executable scripts shipping as part of broctl."),
    Option("PostProcDir", "${BroBase}/share/broctl/scripts/postprocessors", "string", Option.AUTOMATIC, False,
           "Directory for log postprocessors."),
    Option("HelperDir", "${BroBase}/share/broctl/scripts/helpers", "string", Option.AUTOMATIC, False,
           "Directory for broctl helper scripts."),
    Option("CfgDir", "${BroBase}/etc", "string", Option.AUTOMATIC, False,
           "Directory for configuration files."),
    Option("SpoolDir", "${BroBase}/spool", "string", Option.AUTOMATIC, False,
           "Directory for run-time data."),
    Option("PolicyDir", "${BroBase}/share/bro", "string", Option.AUTOMATIC, False,
           "Directory for standard policy files."),
    Option("StaticDir", "${BroBase}/share/broctl", "string", Option.AUTOMATIC, False,
           "Directory for static, arch-independent files."),

    Option("LibDir", "${BroBase}/lib", "string", Option.AUTOMATIC, False,
           "Directory for library files."),
    Option("LibDirInternal", "${BroBase}/lib/broctl", "string", Option.AUTOMATIC, False,
           "Directory for broctl-specific library files."),
    Option("TmpDir", "${SpoolDir}/tmp", "string", Option.AUTOMATIC, False,
           "Directory for temporary data."),
    Option("TmpExecDir", "${SpoolDir}/tmp", "string", Option.AUTOMATIC, False,
           "Directory where binaries are copied before execution."),
    Option("StatsDir", "${LogDir}/stats", "string", Option.AUTOMATIC, False,
           "Directory where statistics are kept."),
    Option("PluginDir", "${LibDirInternal}/plugins", "string", Option.AUTOMATIC, False,
           "Directory where standard plugins are located."),

    Option("TraceSummary", "${bindir}/trace-summary", "string", Option.AUTOMATIC, False,
           "Path to trace-summary script; empty if not available."),
    Option("CapstatsPath", "${bindir}/capstats", "string", Option.AUTOMATIC, False,
           "Path to capstats binary; empty if not available."),

    Option("NodeCfg", "${CfgDir}/node.cfg", "string", Option.AUTOMATIC, False,
           "Node configuration file."),
    Option("LocalNetsCfg", "${CfgDir}/networks.cfg", "string", Option.AUTOMATIC, False,
           "File defining the local networks."),
    Option("StateFile", "${SpoolDir}/broctl.dat", "string", Option.AUTOMATIC, False,
           "File storing the current broctl state."),
    Option("LockFile", "${SpoolDir}/lock", "string", Option.AUTOMATIC, False,
           "Lock file preventing concurrent shell operations."),

    Option("DebugLog", "${SpoolDir}/debug.log", "string", Option.AUTOMATIC, False,
           "Log file for debugging information."),
    Option("StatsLog", "${SpoolDir}/stats.log", "string", Option.AUTOMATIC, False,
           "Log file for statistics."),

    Option("SitePolicyPath", "${PolicyDir}/site", "string", Option.USER, False,
           "Directories to search for local policy files, separated by colons."),
    Option("SitePluginPath", "", "string", Option.USER, False,
           "Directories to search for custom plugins, separated by colons."),


    Option("PolicyDirSiteInstall", "${SpoolDir}/installed-scripts-do-not-touch/site", "string", Option.AUTOMATIC, False,
           "Directory where the shell copies local policy scripts when installing."),
    Option("PolicyDirSiteInstallAuto", "${SpoolDir}/installed-scripts-do-not-touch/auto", "string", Option.AUTOMATIC, False,
           "Directory where the shell copies auto-generated local policy scripts when installing."),

    # Internal, not documented.
    Option("SigInt", "0", "bool", Option.INTERNAL, False,
           "True if SIGINT has been received."),

    Option("Home", "", "string", Option.INTERNAL, False,
           "User's home directory."),

    Option("Cron", "0", "bool", Option.INTERNAL, False,
           "True if we are running from the cron command."),

    Option("BroCtlConfigDir", "${SpoolDir}", "string", Option.INTERNAL, False,
           """Directory where the shell copies the broctl-config.sh
           configuration file. If this is changed, the symlink created in
           CMakeLists.txt must be adapted as well."""),
]

def printOptions(cat):

    for opt in sorted(options, key=lambda o: o.name):

        if opt.category != cat:
            continue

        default = ""

        if not opt.type:
            print >>sys.stderr, "no type given for", opt.name

        if opt.default and opt.type == "string":
            opt.default = '"%s"' % opt.default

        if not opt.default and opt.type == "string":
            opt.default = "_empty_"

        if opt.default:
            default = ", default %s" % opt.default

        default = default.replace("{", "\\{")
        description = opt.description.replace("{", "\\{")

        print ".. _%s:\n\n*%s* (%s%s)\n    %s\n" % (opt.name, opt.name, opt.type, default, description)


if __name__ == '__main__':

    print ".. Automatically generated. Do not edit."
    print
    print "User Options"
    print "~~~~~~~~~~~~"

    printOptions(Option.USER)

    print
    print "Internal Options"
    print "~~~~~~~~~~~~~~~~"
    print

    printOptions(Option.AUTOMATIC)
