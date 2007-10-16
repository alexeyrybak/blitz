--TEST--
has context
--FILE--
<?php
include('common.inc');

$body = <<<BODY
{{ BEGIN alpha }}
{{ BEGIN beta }}
{{ END }}
{{ END }}
BODY;

$T = new Blitz();
$T->load($body);

$ctx_list = array('/alpha', 'alpha', 'beta', '/beta', '/alpha/beta');

echo "/:\n";
foreach($ctx_list as $ctx_item) {
    var_dump($T->has_context($ctx_item));
}

$T->context('alpha');

echo "/alpha:\n";
foreach($ctx_list as $ctx_item) {
    var_dump($T->has_context($ctx_item));
}

?>
--EXPECT--
/:
bool(true)
bool(true)
bool(false)
bool(false)
bool(true)
/alpha:
bool(true)
bool(false)
bool(true)
bool(false)
bool(true)
