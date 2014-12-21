/*
 * Wrappers for swig/python integration
 */

#ifdef	SWIG
%feature("docstring", "Represents the specification of wanted USB device") Spec;
#endif
struct Spec {
#ifdef	SWIG
	%immutable spec;
	%immutable ref_count;
#endif
	struct xusb_spec	*spec;
	int			ref_count;
};

#ifdef	SWIG
%feature("docstring", "Represents a single USB device") XusbDev;
#endif
struct XusbDev {
#ifdef	SWIG
	%immutable spec;
	%immutable xusb_device;
	%immutable ref_count;
#endif
	struct Spec		*spec_wrapper;
	struct xusb_device	*xusb_device;
	int			ref_count;
};

#ifdef	SWIG
%feature("docstring", "Represents a single USB interface") XusbIface;
#endif
struct XusbIface {
#ifdef	SWIG
	%immutable dev_wrapper;
	%immutable iface;
#endif
	struct XusbDev		*dev_wrapper;	/* for ref-counting */
	struct xusb_iface	*iface;
};


