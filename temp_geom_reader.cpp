#include "temp_geom_reader.h"

#include <boost/algorithm/string.hpp>
#include <c++/UDA.hpp>
#include <clientserver/initStructs.h>
#include <clientserver/stringUtils.h>
#include <clientserver/udaStructs.h>
#include <clientserver/udaTypes.h>
#include <fmt/format.h>
#include <ios>
#include <plugins/pluginStructs.h>
#include <plugins/udaPlugin.h>

#include "utils/uda_plugin_helpers.hpp"
#include <deque>
#include <fstream>
// #include <string>

class TempGeomReaderPlugin {
  public:
    void init(IDAM_PLUGIN_INTERFACE* plugin_interface) {
        REQUEST_DATA* request = plugin_interface->request_data;
        if (!init_ || STR_IEQUALS(request->function, "init") || STR_IEQUALS(request->function, "initialise")) {
            reset(plugin_interface);
            // Initialise plugin
            init_ = true;
        }
    }
    void reset(IDAM_PLUGIN_INTERFACE* plugin_interface) {
        if (!init_) {
            // Not previously initialised: Nothing to do!
            return;
        }
        // Free Heap & reset counters
        init_ = false;
    }

    int help(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int version(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int build_date(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int default_method(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int max_interface_version(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int get(IDAM_PLUGIN_INTERFACE* plugin_interface);

  private:
    bool init_ = false;
};

std::deque<std::string> split_request(std::string_view var) {
    std::deque<std::string> split_vec;
    boost::split(split_vec, var, boost::is_any_of("."));
    return split_vec;
}

int tree_check(uda::TreeNode& temp_tree) {

    if (!temp_tree.numChildren()) {
        UDA_LOG(UDA_LOG_DEBUG, "\nimas_json_plugin::plugin_helpers::tree_check: No children found\n");
        return 1;
    }
    if (temp_tree.child(0).name() != "data") {
        UDA_LOG(UDA_LOG_DEBUG, "\nimas_json_plugin::plugin_helpers::tree_check: No child named data\n");
        return 1;
    }
    return 0;
};

std::vector<std::string> get_treenode_child_names(uda::TreeNode& tree) {
    std::vector<std::string> temp_name_vec;
    for (auto& child : tree.children()) {
        temp_name_vec.push_back(child.name());
    }
    return temp_name_vec;
}

int tree_node_traversal(uda::TreeNode& tree, std::deque<std::string>& vec_split) {

    if (vec_split.size() <= 1 or !tree.numChildren()) {
        UDA_LOG(UDA_LOG_DEBUG, "\nimas_json_plugin::plugin_helpers::tree_node_traversal: TreeNode at bottom level\n");
        return 0;
    }

    std::vector<std::string> children{get_treenode_child_names(tree)};
    auto result = std::find(children.begin(), children.end(), vec_split.front());
    if (result != children.end()) {
        tree = tree.child(std::distance(children.begin(), result));
        vec_split.pop_front();
        tree_node_traversal(tree, vec_split);
    } else {
        UDA_LOG(UDA_LOG_DEBUG, "\nimas_json_plugin::plugin_helpers::tree_node_traversal: Child name not found\n");
        return 1;
    }
    return 0;
};

int set_return_data(IDAM_PLUGIN_INTERFACE* interface, uda::TreeNode& final_tree, const std::string& final_var) {

    std::vector<std::string> anames = final_tree.atomicNames();
    std::vector<std::string> atypes = final_tree.atomicTypes();
    std::vector<bool> apoint = final_tree.atomicPointers();
    std::vector<size_t> arank = final_tree.atomicRank();
    std::vector<std::vector<size_t>> ashape = final_tree.atomicShape();

    auto result = std::find(anames.begin(), anames.end(), final_var);
    if (result == anames.end()) {
        return 1;
    }

    long idx{std::distance(anames.begin(), result)};

    // Would be good to use apoint here but seems to be false every time
    if (arank[idx] > 0) {
        if (atypes[idx] == std::string("int")) {
            imas_json_plugin::uda_helpers::setReturnDataArrayType<int>(
                interface->data_block,
                gsl::span<const int>{static_cast<int*>(final_tree.structureComponentData(final_var)), ashape[idx][0]},
                gsl::span<const size_t>{ashape[idx]});
        } else if (atypes[idx] == std::string("float")) {
            imas_json_plugin::uda_helpers::setReturnDataArrayType<float>(
                interface->data_block,
                gsl::span<const float>{static_cast<float*>(final_tree.structureComponentData(final_var)),
                                       ashape[idx][0]},
                gsl::span<const size_t>{ashape[idx]});
        } else if (atypes[idx] == std::string("double")) {
            imas_json_plugin::uda_helpers::setReturnDataArrayType<double>(
                interface->data_block,
                gsl::span<const double>{static_cast<double*>(final_tree.structureComponentData(final_var)),
                                        ashape[idx][0]},
                gsl::span<const size_t>{ashape[idx]});
        } else {
            UDA_LOG(UDA_LOG_DEBUG, "\nimas_json_plugin::plugin_helpers::set_return_data: Unrecognised data type\n");
            return 1;
        }
    } else {
        if (atypes[idx] == std::string("int")) {
            imas_json_plugin::uda_helpers::setReturnDataScalarType<int>(
                interface->data_block, *static_cast<int*>(final_tree.structureComponentData(final_var)));
        } else if (atypes[idx] == std::string("float")) {
            imas_json_plugin::uda_helpers::setReturnDataScalarType<float>(
                interface->data_block, *static_cast<float*>(final_tree.structureComponentData(final_var)));
        } else if (atypes[idx] == std::string("double")) {
            imas_json_plugin::uda_helpers::setReturnDataScalarType<double>(
                interface->data_block, *static_cast<double*>(final_tree.structureComponentData(final_var)));
        } else {
            UDA_LOG(UDA_LOG_DEBUG, "\nimas_json_plugin::plugin_helpers::set_return_data: Unrecognised data type\n");
            return 1;
        }
    }

    return 0;
};

int custom_passive(IDAM_PLUGIN_INTERFACE* interface, uda::TreeNode& final_tree, const std::string& final_var) { 
    return 0;
}

int TempGeomReaderPlugin::get(IDAM_PLUGIN_INTERFACE* interface) {

    //////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////
    DATA_BLOCK* data_block = interface->data_block;
    REQUEST_DATA* request_data = interface->request_data;

    initDataBlock(data_block);
    data_block->rank = 0;
    data_block->dims = nullptr;

    // TODO: put into plugin relevant structure
    int port{0};
    FIND_REQUIRED_INT_VALUE(request_data->nameValueList, port);
    const char* host{nullptr};
    FIND_REQUIRED_STRING_VALUE(request_data->nameValueList, host);
    std::string const host_str{host};

    int source{0};
    FIND_REQUIRED_INT_VALUE(request_data->nameValueList, source);
    const char* signal{nullptr};
    FIND_REQUIRED_STRING_VALUE(request_data->nameValueList, signal);
    std::string signal_str{signal};
    const char* key{nullptr};
    FIND_REQUIRED_STRING_VALUE(request_data->nameValueList, key);
    std::string const key_str{key};

    bool is_custom_passive = findValue(&request_data->nameValueList, "custom_passive");

    static uda::Client client;
    uda::Client::setServerHostName(host_str);
    uda::Client::setServerPort(port);

    // eg. GEOM::get(signal=/magnetics/pfcoil/d1_upper, Config=1);
    std::transform(signal_str.begin(), signal_str.end(), signal_str.begin(), ::tolower);

    std::string geom_request = fmt::format("GEOM::get(signal={}, Config=1)", signal_str);
    // std::string geom_request = fmt::format("GEOM::get(signal={}, cal=1)", signal_str);

    std::deque<std::string> split_vec{split_request(key_str)};
    const uda::Result& data = client.get(geom_request, std::to_string(source));

    // Check for errors
    if (data.errorCode() != uda::OK) {
        RAISE_PLUGIN_ERROR("uda::Result data is not uda::OK");
    }

    // Check this returned data is a structure: Set (register) the data tree for accessors
    if (!data.isTree()) {
        RAISE_PLUGIN_ERROR("Returned data is not of expected tree structure");
    }

    uda::TreeNode root_tree = data.tree();
    // Hack to skip two levels
    if (!tree_check(root_tree)) {
        root_tree = root_tree.child(0);
    }
    if (!tree_check(root_tree)) {
        root_tree = root_tree.child(0);
    }
    
    if (is_custom_passive) {
        return custom_passive(interface, root_tree, split_vec.back());
    }

    if (tree_node_traversal(root_tree, split_vec)) {
        return 1;
    }

    // (0) parse needed arguments
    // (1) access experiment data
    // (2) deduce rank + type (if applicable)
    // (3) set return data (may be dependent on time or data)
    return set_return_data(interface, root_tree, split_vec.front());
}

int TempGeomReader(IDAM_PLUGIN_INTERFACE* plugin_interface) {
    //----------------------------------------------------------------------------------------
    // Standard v1 Plugin Interface

    if (plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        RAISE_PLUGIN_ERROR("Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
    }

    plugin_interface->pluginVersion = THISPLUGIN_VERSION;
    REQUEST_DATA* request = plugin_interface->request_data;

    //----------------------------------------------------------------------------------------
    // Heap Housekeeping

    // Plugin must maintain a list of open file handles and sockets: loop over and close all files and sockets
    // Plugin must maintain a list of plugin functions called: loop over and reset state and free heap.
    // Plugin must maintain a list of calls to other plugins: loop over and call each plugin with the housekeeping
    // request Plugin must destroy lists at end of housekeeping

    // A plugin only has a single instance on a server. For multiple instances, multiple servers are needed.
    // Plugins can maintain state so recursive calls (on the same server) must respect this.
    // If the housekeeping action is requested, this must be also applied to all plugins called.
    // A list must be maintained to register these plugin calls to manage housekeeping.
    // Calls to plugins must also respect access policy and user authentication policy

    try {
        static TempGeomReaderPlugin plugin = {};
        auto* const plugin_func = request->function;

        if (plugin_interface->housekeeping || STR_IEQUALS(plugin_func, "reset")) {
            plugin.reset(plugin_interface);
            return 0;
        }

        //----------------------------------------------------------------------------------------
        // Initialise
        plugin.init(plugin_interface);
        if (STR_IEQUALS(plugin_func, "init") || STR_IEQUALS(plugin_func, "initialise")) {
            return 0;
        }

        //----------------------------------------------------------------------------------------
        // Plugin Functions
        //----------------------------------------------------------------------------------------

        //----------------------------------------------------------------------------------------
        // Standard methods: version, builddate, defaultmethod, maxinterfaceversion

        if (STR_IEQUALS(plugin_func, "help")) {
            return plugin.help(plugin_interface);
        } else if (STR_IEQUALS(plugin_func, "version")) {
            return plugin.version(plugin_interface);
        } else if (STR_IEQUALS(plugin_func, "builddate")) {
            return plugin.build_date(plugin_interface);
        } else if (STR_IEQUALS(plugin_func, "defaultmethod")) {
            return plugin.default_method(plugin_interface);
        } else if (STR_IEQUALS(plugin_func, "maxinterfaceversion")) {
            return plugin.max_interface_version(plugin_interface);
        } else if (STR_IEQUALS(plugin_func, "get")) {
            return plugin.get(plugin_interface);
        } else {
            RAISE_PLUGIN_ERROR("Unknown function requested!");
        }
    } catch (const std::exception& ex) {
        RAISE_PLUGIN_ERROR(ex.what());
    }
}

/**
 * Help: A Description of library functionality
 * @param interface
 * @return
 */
int TempGeomReaderPlugin::help(IDAM_PLUGIN_INTERFACE* interface) {
    const char* help = "\ntemplatePlugin: Add Functions Names, Syntax, and Descriptions\n\n";
    const char* desc = "templatePlugin: help = description of this plugin";

    return setReturnDataString(interface->data_block, help, desc);
}

/**
 * Plugin version
 * @param interface
 * @return
 */
int TempGeomReaderPlugin::version(IDAM_PLUGIN_INTERFACE* interface) {
    return setReturnDataIntScalar(interface->data_block, THISPLUGIN_VERSION, "Plugin version number");
}

/**
 * Plugin Build Date
 * @param interface
 * @return
 */
int TempGeomReaderPlugin::build_date(IDAM_PLUGIN_INTERFACE* interface) {
    return setReturnDataString(interface->data_block, __DATE__, "Plugin build date");
}

/**
 * Plugin Default Method
 * @param interface
 * @return
 */
int TempGeomReaderPlugin::default_method(IDAM_PLUGIN_INTERFACE* interface) {
    return setReturnDataString(interface->data_block, THISPLUGIN_DEFAULT_METHOD, "Plugin default method");
}

/**
 * Plugin Maximum Interface Version
 * @param interface
 * @return
 */
int TempGeomReaderPlugin::max_interface_version(IDAM_PLUGIN_INTERFACE* interface) {
    return setReturnDataIntScalar(interface->data_block, THISPLUGIN_MAX_INTERFACE_VERSION, "Maximum Interface Version");
}
