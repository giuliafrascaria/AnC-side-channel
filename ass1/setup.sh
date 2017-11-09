#!/bin/sh

cp init.sh /etc/init.d/securesoftware.sh
chmod +x /etc/init.d/securesoftware.sh
update-rc.d securesoftware.sh defaults

