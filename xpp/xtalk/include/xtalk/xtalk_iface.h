/*
 * Wrappers for swig/python integration
 */

struct Command {
	struct xtalk_command	*command;
};

struct Xtalksync {
	struct xtalk_base	*xtalk_base;
	struct xtalk_sync	*xtalk_sync;
	struct XusbIface	*py_xusb_iface;
};

struct Xtalkraw {
	struct xtalk_base	*xtalk_base;
	struct xtalk_raw	*xtalk_raw;
	struct XusbIface	*py_xusb_iface;
};
