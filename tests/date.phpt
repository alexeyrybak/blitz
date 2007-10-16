--TEST--
date output wrapper
--FILE--
<?php
include('common.inc');

if (function_exists('date_default_timezone_set'))
    date_default_timezone_set('UTC');

$body = <<<BODY
{{ date("%d %m %Y %H:%M:%S",\$time_num); }}
{{ date("%d %m %Y %H:%M:%S",\$time_str); }}
{{ date("%d %m %Y %H:%M:%S"); }}

BODY;

$T = new Blitz();
$T->load($body);

$time_num = mktime(12, 1, 1, 7, 22, 1976);
echo date("d m Y h:i:s", $time_num)."\n";
$time_str = '1976-07-22 12:01:01';
$T->set(array(
    'time_num' => $time_num,
    'time_str' => $time_str
));
echo $T->parse();

?>
--EXPECTREGEX--
22 07 1976 12:01:01
22 07 1976 12:01:01
22 07 1976 12:01:01
\d{2} \d{2} \d{4} \d{2}:\d{2}:\d{2}
