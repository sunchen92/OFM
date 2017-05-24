# Functions to control the nodes' operations.

import os
import sys
import glob
import fileinput
import time
import tempfile
import re

import execute
import util
import config
import cron
import install
import plugin
import node as node_mod

# Convert a number into a string with a unit (e.g., 1024 into "1M").
def prettyPrintVal(val):
    for (prefix, unit, factor) in (("", "G", 1024*1024*1024), ("", "M", 1024*1024), ("", "K", 1024)):
        if val >= factor:
            return "%s%3.0f%s" % (prefix, val / factor, unit)
    return " %3.0f" % (val)

# Checks multiple nodes in parallel and returns list of tuples (node, isrunning).
def isRunning(nodes, setcrashed=True):

    results = []
    cmds = []

    for node in nodes:
        pid = node.getPID()
        if not pid:
            results += [(node, False)]
            continue

        cmds += [(node, "check-pid", [str(pid)])]

    for (node, success, output) in execute.runHelperParallel(cmds):

        # If we cannot connect to the host at all, we filter it out because
        # the process might actually still be running but we can't tell.
        if output == None:
            if config.Config.cron == "0":
                util.warn("cannot connect to %s" % node.name)
            continue

        results += [(node, success)]

        if not success:
            if setcrashed:
                # Grmpf. It crashed.
                node.clearPID();
                node.setCrashed()

    return results

# For a list of (node, bool), returns True if at least one boolean is False.
def nodeFailed(nodes):
    for (node, success) in nodes:
        if not success:
            return True
    return False

# Waits for the nodes' Bro processes to reach the given status.
def waitForBros(nodes, status, timeout, ensurerunning):

    # If ensurerunning is true, process must still be running.
    if ensurerunning:
        running = isRunning(nodes)
    else:
        running = [(node, True) for node in nodes]

    results = []

    # Determine set of nodes still to check.
    todo = {}
    for (node, isrunning) in running:
        if isrunning:
            todo[node.name] = node
        else:
            results += [(node, False)]

    more_than_one = (len(todo) > 1)

    points = False
    while True:
        # Determine  whether process is still running. We need to do this
        # before we get the state to avoid a race condition.
        running = isRunning(todo.values(), setcrashed=False)

        # Check nodes' .status file
        cmds = []
        for node in todo.values():
            cmds += [(node, "cat-file", ["%s/.status" % node.cwd()])]

        for (node, success, output) in execute.runHelperParallel(cmds):
            if success:
                try:
                    (stat, loc) = output[0].split()
                    if status in stat:
                        # Status reached. Cool.
                        del todo[node.name]
                        results += [(node, True)]
                except IndexError:
                    # Something's wrong. We give up on that node.
                    del todo[node.name]
                    results += [(node, False)]

        for (node, isrunning) in running:
            if node.name in todo and not isrunning:
                # Alright, a dead node's status will not change anymore.
                del todo[node.name]
                results += [(node, False)]

        if len(todo) == 0:
            # All done.
            break

        # Wait a bit before we start over.
        time.sleep(1)

        # Timeout reached?
        timeout -= 1
        if timeout <= 0:
            break

        if more_than_one:
            util.output("%d " % len(todo), nl=False)
        else:
            util.output(".", nl=False)

        points = True

    for node in todo.values():
        # These did time-out.
        results += [(node, False)]

    if points:
        if more_than_one:
            util.output("%d " % len(todo))
        else:
            util.output("")

    return results

# Build the Bro parameters for the given node. Include
# script for live operation if live is true.
def _makeBroParams(node, live, add_manager=False):
    args = []

    if live and node.interface:
        try:
            args += ["-i %s " % node.interface]
        except AttributeError:
            pass

        if config.Config.savetraces == "1":
            args += ["-w trace.pcap"]

    args += ["-U .status"]
    args += ["-p broctl"]

    if live:
        args += ["-p broctl-live"]

    if node.type == "standalone":
        args += ["-p standalone"]

    for p in config.Config.prefixes.split(":"):
        args += ["-p %s" % p]

    args += ["-p %s" % node.name]

    # The order of loaded scripts is as follows:
    # 1) local.bro gives a common set of loaded scripts for all nodes.
    # 2) The common configuration of broctl is loaded via the broctl package.
    # 3) The distribution's default settings for node configuration are loaded
    #    from either the cluster framework or standalone scripts.  This also
    #    involves loading local-<node>.bro scripts.  At this point anything
    #    in the distribution's default per-node is overridable and any
    #    identifiers in local.bro are able to be used (e.g. in defining
    #    a notice policy).
    # 4) Autogenerated broctl scripts are loaded, which may contain
    #    settings that override the previously loaded node-specific scripts.
    #    (e.g. Log::default_rotation_interval is set in manager.bro,
    #    but overrided by broctl.cfg)
    args += config.Config.sitepolicystandalone.split()
    args += ["broctl"]
    if node.type == "standalone":
        args += ["broctl/standalone"]
    else:
        args += ["base/frameworks/cluster"]
        if node.type == "manager":
            args += config.Config.sitepolicymanager.split()
        elif node.type == "proxy":
            args += ["local-proxy"]
        elif node.type == "worker":
            args += config.Config.sitepolicyworker.split()
    args += ["broctl/auto"]

    if "aux_scripts" in node.__dict__:
        args += [node.aux_scripts]

    if config.Config.broargs:
        args += [config.Config.broargs]

#   args += ["-B comm,serial"]

    return args

# Build the environment variable for the given node.
def _makeEnvParam(node):
    env = ""
    if node.type != "standalone":
        env += " CLUSTER_NODE=%s" % node.name

    for env_var in node.env_vars:
        env += " %s" % env_var

    return env

# Do a "post-terminate crash" for the given nodes.
def _makeCrashReports(nodes):

    for n in nodes:
        plugin.Registry.broProcessDied(n)

    cmds = []
    for node in nodes:
        cmds += [(node, "run-cmd",  [os.path.join(config.Config.scriptsdir, "post-terminate"), node.cwd(),  "crash"])]

    for (node, success, output) in execute.runHelperParallel(cmds):
        if not success:
            util.output("cannot run post-terminate for %s" % node.name)
        else:
            util.sendMail("Crash report from %s" % node.name, "\n".join(output))

        node.clearCrashed()

# Starts the given nodes.
def _startNodes(nodes):
    results = []

    filtered = []
    # Ignore nodes which are still running.
    for (node, isrunning) in isRunning(nodes):
        if not isrunning:
            filtered += [node]
            util.output("starting %s ..." % node.name)
        else:
            util.output("%s still running" % node.name)

    nodes = filtered

    # Generate crash report for any crashed nodes.
    crashed = [node for node in nodes if node.hasCrashed()]
    _makeCrashReports(crashed)

    # Make working directories.
    dirs = [(node, node.cwd()) for node in nodes]
    nodes = []
    for (node, success) in execute.mkdirs(dirs):
        if success:
            nodes += [node]
        else:
            util.output("cannot create working directory for %s" % node.name)
            results += [(node, False)]

    # Start Bro process.
    cmds = []
    envs = []
    for node in nodes:
        cmds += [(node, "start", [node.cwd()] + _makeBroParams(node, True))]
        envs += [_makeEnvParam(node)]

    nodes = []
    for (node, success, output) in execute.runHelperParallel(cmds, envs=envs):
        if success:
            nodes += [node]
            node.setPID(int(output[0]))
        else:
            util.output("cannot start %s" % node.name)
            results += [(node, False)]

    # Check whether processes did indeed start up.
    hanging = []
    running = []

    for (node, success) in waitForBros(nodes, "RUNNING", 3, True):
        if success:
            running += [node]
        else:
            hanging += [node]

    # It can happen that Bro hangs in DNS lookups at startup
    # which can take a while. At this point we already know
    # that the process has been started (waitForBro ensures that).
    # If by now there is not a TERMINATED status, we assume that it
    # is doing fine and will move on to RUNNING once DNS is done.
    for (node, success) in waitForBros(hanging, "TERMINATED", 0, False):
        if success:
            util.output("%s terminated immediately after starting; check output with \"diag\"" % node.name)
            node.clearPID()
            results += [(node, False)]
        else:
            util.output("(%s still initializing)" % node.name)
            running += [node]

    for node in running:
        cron.logAction(node, "started")
        results += [(node, True)]

    return results

# Start Bro processes on nodes if not already running.
def start(nodes):
    if len(nodes) > 0:
        # User picked nodes to start.
        return _startNodes(nodes)

    manager = config.Config.nodes("manager")
    proxies = config.Config.nodes("proxies")
    workers = config.Config.nodes("workers")

    # Start all nodes. Do it in the order manager, proxies, workers.

    results1 = _startNodes(manager)

    if nodeFailed(results1):
        return results1 + [(n, False) for n in (proxies + workers)]

    results2 = _startNodes(proxies)

    if nodeFailed(results2):
        return results1 + results2 + [(n, False) for n in workers]

    results3 = _startNodes(workers)

    return results1 + results2 + results3

def _stopNodes(nodes):

    results = []
    running = []

    # Check for crashed nodes.
    for (node, isrunning) in isRunning(nodes):
        if isrunning:
            running += [node]
            util.output("stopping %s ..." % node.name)
        else:
            results += [(node, True)]

            if node.hasCrashed():
                _makeCrashReports([node])
                util.output("%s not running (was crashed)" % node.name)
            else:
                util.output("%s not running" % node.name)

    # Helper function to stop nodes with given signal.
    def stop(nodes, signal):
        cmds = []
        for node in nodes:
            cmds += [(node, "stop", [str(node.getPID()), str(signal)])]

        return execute.runHelperParallel(cmds)
        #events = []
        #for node in nodes:
        #    events += [(node, "Control::shutdown_request", [], "Control::shutdown_response")]
        #return execute.sendEventsParallel(events)

    # Stop nodes.
    for (node, success, output) in stop(running, 15):
        if not success:
            util.output("failed to send stop signal to %s" % node.name)

    if running:
        time.sleep(1)

    # Check whether they terminated.
    terminated = []
    kill = []
    for (node, success) in waitForBros(running, "TERMINATED", int(config.Config.stoptimeout), False):
        if not success:
            # Check whether it crashed during shutdown ...
            result = isRunning([node])
            for (node, isrunning) in result:
                if isrunning:
                    util.output("%s did not terminate ... killing ..." % node.name)
                    kill += [node]
                else:
                    # crashed flag is set by isRunning().
                    util.output("%s crashed during shutdown" % node.name)

    if len(kill):
        # Kill those which did not terminate gracefully.
        stop(kill, 9)
        # Given them a bit to disappear.
        time.sleep(5)

    # Check which are still running. We check all nodes to be on the safe side
    # and give them a bit more time to finally disappear.
    timeout = 10

    todo = {}
    for node in running:
        todo[node.name] = node

    while True:

        running = isRunning(todo.values(), setcrashed=False)

        for (node, isrunning) in running:
            if node.name in todo and not isrunning:
                # Alright, it's gone.
                del todo[node.name]
                terminated += [node]
                results += [(node, True)]

        if len(todo) == 0:
            # All done.
            break

        # Wait a bit before we start over.

        if timeout <= 0:
            break

        time.sleep(1)
        timeout -= 1

    results += [(node, False) for node in todo]

    # Do post-terminate cleanup for those which terminated gracefully.
    cleanup = [node for node in terminated if not node.hasCrashed()]

    cmds = []
    for node in cleanup:
        cmds += [(node, "run-cmd",  [os.path.join(config.Config.scriptsdir, "post-terminate"), node.cwd()])]

    for (node, success, output) in execute.runHelperParallel(cmds):
        if not success:
            util.output("cannot run post-terminate for %s" % node.name)
            cron.logAction(node, "stopped (failed)")
        else:
            cron.logAction(node, "stopped")

        node.clearPID()
        node.clearCrashed()

    return results

# Stop Bro processes on nodes.
def stop(nodes):
    if len(nodes) > 0:
        # User picked nodes to stop.
        return _stopNodes(nodes)

    manager = config.Config.nodes("manager")
    proxies = config.Config.nodes("proxies")
    workers = config.Config.nodes("workers")

    # Stop all nodes. Do it in the order workers, proxies, manager
    # (the reverse of "start").

    results1 = _stopNodes(workers)

    if nodeFailed(results1):
        return results1 + [(n, False) for n in (proxies + manager)]

    results2 = _stopNodes(proxies)

    if nodeFailed(results2):
        return results1 + results2 + [(n, False) for n in manager]

    results3 = _stopNodes(manager)

    return results1 + results2 + results3

# Output status summary for nodes.
def status(nodes):

    util.output("%-10s %-10s %-10s %-13s %-6s %-6s %-20s " % ("Name",  "Type", "Host", "Status", "Pid", "Peers", "Started"))

    all = isRunning(nodes)
    running = []

    cmds1 = []
    cmds2 = []
    for (node, isrunning) in all:
        if isrunning:
            running += [node]
            cmds1 += [(node, "cat-file", ["%s/.startup" % node.cwd()])]
            cmds2 += [(node, "cat-file", ["%s/.status" % node.cwd()])]

    startups = execute.runHelperParallel(cmds1)
    statuses = execute.runHelperParallel(cmds2)

    startups = dict([(n.name, success and util.fmttime(output[0]) or "???") for (n, success, output) in startups])
    statuses = dict([(n.name, success and output[0].split()[0].lower() or "???") for (n, success, output) in statuses])

    peers = {}
    nodes = [n for n in running if statuses[n.name] == "running"]
    for (node, success, args) in _queryPeerStatus(nodes):
        if success:
            peers[node.name] = []
            for f in args[0].split():
                keyval = f.split("=")
                if len(keyval) > 1:
                    (key, val) = keyval
                    if key == "peer" and val != "":
                        peers[node.name] += [val]
        else:
            peers[node.name] = None

    for (node, isrunning) in all:

        util.output("%-10s " % node.name, nl=False)
        util.output("%-10s %-10s " % (node.type, node.host), nl=False)

        if isrunning:
            util.output("%-13s " % statuses[node.name], nl=False)

        elif node.hasCrashed():
            util.output("%-13s " % "crashed", nl=False)
        else:
            util.output("%-13s " % "stopped", nl=False)

        if isrunning:
            util.output("%-6s " % node.getPID(), nl=False)

            if node.name in peers and peers[node.name] != None:
                util.output("%-6d " % len(peers[node.name]), nl=False)
            else:
                util.output("%-6s " % "???", nl=False)

            util.output("%-8s  " % startups[node.name], nl=False)

        util.output()

# Outputs state of remote connections for host.


# Helper for getting top output.
#
# Returns tuples of the form (node, error, vals) where  'error' is None if we
# were able to get the data or otherwise a string with an  error message;
# in case there's no error, 'vals' is a list of dicts which map tags to their values.
#
# Tags are "pid", "proc", "vsize", "rss", "cpu", and "cmd".
#
# We do all the stuff in parallel across all nodes which is why this looks
# a bit confusing ...
def getTopOutput(nodes):

    results = []
    cmds = []

    running = isRunning(nodes)

    # Get all the PIDs first.

    pids = {}
    parents = {}

    for (node, isrunning) in running:
        if isrunning:
            pid = node.getPID()
            pids[node.name] = [pid]
            parents[node.name] = str(pid)

            cmds += [(node, "get-childs", [str(pid)])]
        else:
            results += [(node, "not running", [{}])]
            continue

    if not cmds:
        return results

    for (node, success, output) in execute.runHelperParallel(cmds):

        if not success:
            results += [(node, "cannot get child pids", [{}])]
            continue

        pids[node.name] += [int(line) for line in output]

    cmds = []

    # Now run top.
    for node in nodes: # Do the loop again to keep the order.
        if not node.name in pids:
            continue

        cmds += [(node, "top", [])]

    if not cmds:
        return results

    for (node, success, output) in execute.runHelperParallel(cmds):

        if not success:
            results += [(node, "cannot get top output", [{}])]

        procs = [line.split() for line in output if int(line.split()[0]) in pids[node.name]]

        if not procs:
            # It can happen that on the meantime the process is not there anymore.
            results += [(node, "not running", [{}])]
            continue

        vals = []

        for p in procs:
            d = {}
            d["pid"] = int(p[0])
            d["proc"] = (p[0] == parents[node.name] and "parent" or "child")
            d["vsize"] = long(float(p[1])) # May be something like 2.17684e+09
            d["rss"] = long(float(p[2]))
            d["cpu"] = p[3]
            d["cmd"] = " ".join(p[4:])
            vals += [d]

        results += [(node, None, vals)]

    return results

# Produce a top-like output for node's processes.
# If hdr is true, output column headers first.
def top(nodes):

    util.output("%-10s %-10s %-10s %-8s %-8s %-8s %-8s %-8s %-8s" % ("Name", "Type", "Node", "Pid", "Proc", "VSize", "Rss", "Cpu", "Cmd"))

    for (node, error, vals) in getTopOutput(nodes):

        if not error:
            for d in vals:
                util.output("%-10s " % node.name, nl=False)
                util.output("%-10s " % node.type, nl=False)
                util.output("%-10s " % node.host, nl=False)
                util.output("%-8s " % d["pid"], nl=False)
                util.output("%-8s " % d["proc"], nl=False)
                util.output("%-8s " % prettyPrintVal(d["vsize"]), nl=False)
                util.output("%-8s " % prettyPrintVal(d["rss"]), nl=False)
                util.output("%-8s " % ("%s%%" % d["cpu"]), nl=False)
                util.output("%-8s " % d["cmd"], nl=False)
                util.output()
        else:
            util.output("%-10s " % node.name, nl=False)
            util.output("%-8s " % node.type, nl=False)
            util.output("%-8s " % node.host, nl=False)
            util.output("<%s> " % error, nl=False)
            util.output()

def _doCheckConfig(nodes, installed, list_scripts):

    results = []

    manager = config.Config.manager()

    all = [(node, os.path.join(config.Config.tmpdir, "check-config-%s" % node.name)) for node in nodes]

    nodes = []
    for (node, cwd) in all:
        if os.path.isdir(cwd):
            if not execute.rmdir(config.Config.manager(), cwd):
                util.output("cannot remove directory %s on manager" % cwd)
                continue

        if not execute.mkdir(config.Config.manager(), cwd):
            util.output("cannot create directory %s on manager" % cwd)
            continue

        nodes += [(node, cwd)]

    cmds = []
    for (node, cwd) in nodes:

        env = _makeEnvParam(node)

        installed_policies = installed and "1" or "0"
        print_scripts = list_scripts and "1" or "0"

        install.makeLayout(cwd, True)
        install.makeLocalNetworks(cwd, True)
        install.makeConfig(cwd, True)

        cmd = os.path.join(config.Config.scriptsdir, "check-config") + " %s %s %s %s" % (installed_policies, print_scripts, cwd, " ".join(_makeBroParams(node, False)))
        cmd += " broctl/check"

        cmds += [((node, cwd), cmd, env, None)]

    for ((node, cwd), success, output) in execute.runLocalCmdsParallel(cmds):
        if success:
            util.output("%s is ok." % node.name)
            if list_scripts:
                for line in output:
                    util.output("  %s" % line)
        else:
            ok = False
            util.output("%s failed." % node.name)
            for line in output:
                util.output("   %s" % line)

        execute.rmdir(manager, cwd)

    return results

# Check the configuration for nodes without installing first.
def checkConfigs(nodes):
    return _doCheckConfig(nodes, False, False)

# Prints the loaded_scripts.log for either the installed scripts
# (if check argument is false), or the original scripts (if check arg is true)
def listScripts(nodes, check):
    _doCheckConfig(nodes, not check, True)

# Report diagostics for node (e.g., stderr output).
def crashDiag(node):

    util.output("[%s]" % node.name)

    if not execute.isdir(node, node.cwd()):
        util.output("No work dir found\n")
        return

    (rc, output) = execute.runHelper(node, "run-cmd",  [os.path.join(config.Config.scriptsdir, "crash-diag"), node.cwd()])
    if not rc:
        util.output("cannot run crash-diag for %s" % node.name)
        return

    for line in output:
        util.output(line)

# Clean up the working directory for nodes (flushes state).
# If cleantmp is true, also wipes ${tmpdir}; this is done
# even when the node is still running.
def cleanup(nodes, cleantmp=False):
    util.output("cleaning up nodes ...")
    result = isRunning(nodes)
    running =    [node for (node, on) in result if on]
    notrunning = [node for (node, on) in result if not on]

    execute.rmdirs([(n, n.cwd()) for n in notrunning])
    execute.mkdirs([(n, n.cwd()) for n in notrunning])

    for node in notrunning:
        node.clearCrashed();

    for node in running:
        util.output("   %s is still running, not cleaning work directory" % node.name)

    if cleantmp:
        execute.rmdirs([(n, config.Config.tmpdir) for n in running + notrunning])
        execute.mkdirs([(n, config.Config.tmpdir) for n in running + notrunning])

# Attach gdb to the main Bro processes on the given nodes.
def attachGdb(nodes):
    running = isRunning(nodes)

    cmds = []
    for (node, isrunning) in running:
        if isrunning:
            cmds += [(node, "gdb-attach", ["gdb-%s" % node.name, config.Config.bro, str(node.getPID())])]

    results = execute.runHelperParallel(cmds)
    for (node, success, output) in results:
        if success:
            util.output("gdb attached on %s" % node.name)
        else:
            util.output("cannot attach gdb on %s: %s" % node.name, output)

# Helper for getting capstats output.
#
# Returns tuples of the form (node, error, vals) where  'error' is None if we
# were able to get the data or otherwise a string with an error message;
# in case there's no error, 'vals' maps tags to their values.
#
# Tags are those as returned by capstats on the command-line
#
# There is one "pseudo-node" of the name "$total" with the sum of all
# individual values.
#
# We do all the stuff in parallel across all nodes which is why this looks
# a bit confusing ...

# Gather capstats from interfaces.
def getCapstatsOutput(nodes, interval):

    if not config.Config.capstatspath:
        if config.Config.cron == "0":
            util.warn("do not have capstats binary available")
        return []

    results = []
    cmds = []

    hosts = {}
    for node in nodes:
        try:
            hosts[(node.addr, node.interface)] = node
        except AttributeError:
            continue

    for (addr, interface) in hosts.keys():
        node = hosts[addr, interface]

        capstats = [config.Config.capstatspath, "-i", interface, "-I", str(interval), "-n", "1"]

# Unfinished feature: only consider a particular MAC. Works here for capstats
# but Bro config is not adapted currently so we disable it for now.
#        try:
#            capstats += ["-f", "\\'", "ether dst %s" % node.ether, "\\'"]
#        except AttributeError:
#            pass

        cmds += [(node, "run-cmd", capstats)]

    outputs = execute.runHelperParallel(cmds)

    totals = {}

    for (node, success, output) in outputs:

        if not success:
            results += [(node, "%s: cannot execute capstats" % node.name, {})]
            continue

        fields = output[0].split()
        vals = { }

        try:
            for field in fields[1:]:
                (key, val) = field.split("=")
                val = float(val)
                vals[key] = val

                try:
                    totals[key] += val
                except KeyError:
                    totals[key] = val

            results += [(node, None, vals)]

        except ValueError:
            results += [(node, "%s: unexpected capstats output: %s" % (node.name, output[0]), {})]

    # Add pseudo-node for totals
    if len(nodes) > 1:
        results += [(node_mod.Node("$total"), None, totals)]

    return results

# Get current statistics from cFlow.
#
# Returns dict of the form port->(cum-pkts, cum-bytes).
#
# Returns None if we can't run the helper sucessfully.
def getCFlowStatus():
    (success, output) = execute.runLocalCmd(os.path.join(config.Config.scriptsdir, "cflow-stats"))
    if not success or not output:
        util.warn("failed to run cflow-stats")
        return None

    vals = {}

    for line in output:
        try:
            (port, pps, bps, pkts, bytes) = line.split()
            vals[port] = (float(pkts), float(bytes))
        except ValueError:
            # Probably an error message because we can't connect.
            util.warn("failed to get cFlow statistics: %s" % line)
            return None

    return vals

# Calculates the differences between to getCFlowStatus() calls.
# Returns tuples in the same form as getCapstatsOutput() does.
def calculateCFlowRate(start, stop, interval):
    diffs = [(port, stop[port][0] - start[port][0], (stop[port][1] - start[port][1])) for port in start.keys() if port in stop]

    rates = []
    for (port, pkts, bytes) in diffs:
        vals = { "kpps": "%.1f" % (pkts / 1e3 / interval) }
        if start[port][1] >= 0:
            vals["mbps"] = "%.1f" % (bytes * 8 / 1e6 / interval)

        rates += [(port, None, vals)]

    return rates

def capstats(nodes, interval):

    def output(tag, data):

        def outputOne(tag, vals):
            util.output("%-20s " % tag, nl=False)

            if not error:
                util.output("%-10s " % vals["kpps"], nl=False)
                if "mbps" in vals:
                    util.output("%-10s " % vals["mbps"], nl=False)
                util.output()
            else:
                util.output("<%s> " % error)

        util.output("\n%-20s %-10s %-10s (%ds average)" % (tag, "kpps", "mbps", interval))
        util.output("-" * 30)

        totals = None

        for (port, error, vals) in sorted(data):

            if error:
                util.output(error)
                continue

            if str(port) != "$total":
                outputOne(port, vals)
            else:
                totals = vals

        if totals:
            util.output("")
            outputOne("Total", totals)
            util.output("")

    have_cflow = config.Config.cflowaddress and config.Config.cflowuser and config.Config.cflowpassword
    have_capstats = config.Config.capstatspath

    if not have_cflow and not have_capstats:
        util.warn("do not have capstats binary available")
        return

    if have_cflow:
        cflow_start = getCFlowStatus()

    if have_capstats:
        capstats = []
        for (node, error, vals) in getCapstatsOutput(nodes, interval):
            if str(node) == "$total":
                capstats += [(node, error, vals)]
            elif node.interface:
                capstats += [("%s/%s" % (node.host, node.interface), error, vals)]

    else:
        time.sleep(interval)

    if have_cflow:
        cflow_stop = getCFlowStatus()

    if have_capstats:
        output("Interface", sorted(capstats))

    if have_cflow and cflow_start and cflow_stop:
        diffs = calculateCFlowRate(cflow_start, cflow_stop, interval)
        output("cFlow Port", sorted(diffs))

# Update the configuration of a running instance on the fly.
def update(nodes):

    running = isRunning(nodes)
    zone = config.Config.zoneid
    if zone == "":
        zone = "NOZONE"

    cmds = []
    for (node, isrunning) in running:
        if isrunning:
            env = _makeEnvParam(node)
            env += " BRO_DNS_FAKE=1"
            args = " ".join(_makeBroParams(node, False))
            cmds += [(node.name, os.path.join(config.Config.scriptsdir, "update") + " %s %s %s/tcp %s" % (util.formatBroAddr(node.addr), zone, node.getPort(), args), env, None)]
            util.output("updating %s ..." % node.name)

    results = execute.runLocalCmdsParallel(cmds)

    for (tag, success, output) in results:
        if not success:
            util.output("could not update %s: %s" % (tag, output))
        else:
            util.output("%s: %s" % (tag, output[0]))

    return [(config.Config.nodes(tag=tag)[0], success) for (tag, success, output) in results]

# Gets disk space on all volumes relevant to broctl installation.
# Returns dict which for each node has a list of tuples (fs, total, used, avail).
def getDf(nodes):

    dirs = ("logdir", "bindir", "helperdir", "cfgdir", "spooldir", "policydir", "libdir", "tmpdir", "staticdir", "scriptsdir")

    df = {}
    for node in nodes:
        df[node.name] = {}

    for dir in dirs:
        path = config.Config.config[dir]

        cmds = []
        for node in nodes:
            if dir == "logdir" and node.type != "manager":
                # Don't need this on the workers/proxies.
                continue

            cmds += [(node, "df", [path])]

        results = execute.runHelperParallel(cmds)

        for (node, success, output) in results:
            if success:
                if len(output) > 0:
                    fields = output[0].split()

                    # Ignore NFS mounted volumes.
                    if fields[0].find(":") < 0:
                        df[node.name][fields[0]] = fields
                else:
                    util.warn("Invalid df output for node '%s'." % node)


    result = {}
    for node in df:
        result[node] = df[node].values()

    return result

def df(nodes):

    util.output("%10s  %15s  %-5s  %-5s  %-5s" % ("", "", "total", "avail", "capacity"))

    for (node, dfs) in getDf(nodes).items():
        for df in dfs:
            total = float(df[1])
            used = float(df[2])
            avail = float(df[3])
            perc = used * 100.0 / (used + avail)

            util.output("%10s  %15s  %-5s  %-5s  %-5.1f%%" % (node, df[0],
                prettyPrintVal(total),
                prettyPrintVal(avail), perc))


def printID(nodes, id):
    running = isRunning(nodes)

    events = []
    for (node, isrunning) in running:
        if isrunning:
            events += [(node, "Control::id_value_request", [id], "Control::id_value_response")]

    results = execute.sendEventsParallel(events)

    for (node, success, args) in results:
        if success:
            print "%10s   %s = %s" % (node, args[0], args[1])
        else:
            print "%10s   <error: %s>" % (node, args)

def _queryPeerStatus(nodes):
    running = isRunning(nodes)

    events = []
    for (node, isrunning) in running:
        if isrunning:
            events += [(node, "Control::peer_status_request", [], "Control::peer_status_response")]

    return execute.sendEventsParallel(events)

def _queryNetStats(nodes):
    running = isRunning(nodes)

    events = []
    for (node, isrunning) in running:
        if isrunning:
            events += [(node, "Control::net_stats_request", [], "Control::net_stats_response")]

    return execute.sendEventsParallel(events)

def peerStatus(nodes):
    for (node, success, args) in _queryPeerStatus(nodes):
        if success:
            print "%10s\n%s" % (node, args[0])
        else:
            print "%10s   <error: %s>" % (node, args)

def netStats(nodes):
    for (node, success, args) in _queryNetStats(nodes):
        if success:
            print "%10s: %s" % (node, args[0]),
        else:
            print "%10s: <error: %s>" % (node, args)

def executeCmd(nodes, cmd):
    for (node, success, output) in execute.executeCmdsParallel([(n, cmd) for n in nodes]):
        util.output("[%s] %s\n> %s" % (node.name, (success and " " or "error"), "\n> ".join(output)))

def processTrace(trace, bro_options, bro_scripts):
    standalone = (config.Config.standalone == "1")
    if standalone:
        tag = "standalone"
    else:
        tag = "workers"

    node = config.Config.nodes(tag=tag)[0]

    cwd = os.path.join(config.Config.tmpdir, "testing")

    if not execute.rmdir(config.Config.manager(), cwd):
        util.output("cannot remove directory %s on manager" % cwd)
        return False

    if not execute.mkdir(config.Config.manager(), cwd):
        util.output("cannot create directory %s on manager" % cwd)
        return False

    env = _makeEnvParam(node)

    bro_args =  " ".join(bro_options + _makeBroParams(node, False, add_manager=(not standalone)))

    if bro_scripts:
        bro_args += " " + " ".join(bro_scripts)

    cmd = os.path.join(config.Config.scriptsdir, "run-bro-on-trace") + " %s %s %s %s" % (0, cwd, trace, bro_args)
    cmd += " broctl/process-trace"

    print cmd

    (success, output) = execute.runLocalCmd(cmd, env, donotcaptureoutput=True)

    for line in output:
        util.output(line)

    util.output("")
    util.output("### Bro output in %s" % cwd)

    return success

