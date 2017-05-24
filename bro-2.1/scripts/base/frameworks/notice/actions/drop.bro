##! This script extends the built in notice code to implement the IP address
##! dropping functionality.

@load ../main

module Notice;

export {
	redef enum Action += {
		## Drops the address via Drop::drop_address, and generates an alarm.
		ACTION_DROP
	};

	redef record Info += {
		## Indicate if the $src IP address was dropped and denied network access.
		dropped:  bool           &log &default=F;
	};
}

# This is a little awkward because we want to inject drop along with the
# synchronous functions.
event bro_init()
	{
	local drop_func = function(n: Notice::Info)
		{
		if ( ACTION_DROP in n$actions )
			{
			#local drop = React::drop_address(n$src, "");
			#local addl = drop?$sub ? fmt(" %s", drop$sub) : "";
			#n$dropped = drop$note != Drop::AddressDropIgnored;
			#n$msg += fmt(" [%s%s]", drop$note, addl);
			}
		};

	add Notice::sync_functions[drop_func];
	}
