##! Interface for the ASCII log writer.  Redefinable options are available
##! to tweak the output format of ASCII logs.

module LogAscii;

export {
	## If true, output everything to stdout rather than
	## into files. This is primarily for debugging purposes.
	const output_to_stdout = F &redef;

	## If true, include lines with log meta information such as column names with
	## types, the values of ASCII logging options that in use, and the time when the
	## file was opened and closes (the latter at the end). 
	const include_meta = T &redef;

	## Prefix for lines with meta information.
	const meta_prefix = "#" &redef;

	## Separator between fields.
	const separator = "\t" &redef;

	## Separator between set elements.
	const set_separator = "," &redef;

	## String to use for empty fields. This should be different from
        ## *unset_field* to make the output non-ambigious. 
	const empty_field = "(empty)" &redef;

	## String to use for an unset &optional field.
	const unset_field = "-" &redef;
}

# Default function to postprocess a rotated ASCII log file. It moves the rotated
# file to a new name that includes a timestamp with the opening time, and then
# runs the writer's default postprocessor command on it.
function default_rotation_postprocessor_func(info: Log::RotationInfo) : bool
	{
	# Move file to name including both opening and closing time.
	local dst = fmt("%s.%s.log", info$path,
			strftime(Log::default_rotation_date_format, info$open));

	system(fmt("/bin/mv %s %s", info$fname, dst));

	# Run default postprocessor.
	return Log::run_rotation_postprocessor_cmd(info, dst);
	}

redef Log::default_rotation_postprocessors += { [Log::WRITER_ASCII] = default_rotation_postprocessor_func };
