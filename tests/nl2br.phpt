--TEST--
nl2br filter
--FILE--
<?php
include('common.inc');

$str = <<<STR
<html><head>bla</head></html>
<html><head>bla</head></html>
STR;

$body = <<<BODY
nl2br'd code:
{{ \$str | nl2br }}
BODY;


$T = new Blitz();
$T->load($body);


$T->display(array('str' => $str))

?>
--EXPECT--
nl2br'd code:
&lt;html&gt;&lt;head&gt;bla&lt;/head&gt;&lt;/html&gt;<br />
&lt;html&gt;&lt;head&gt;bla&lt;/head&gt;&lt;/html&gt;
