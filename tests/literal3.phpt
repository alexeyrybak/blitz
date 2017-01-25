--TEST--
literal #3
--FILE--
<?php

include('common.inc');
ini_set('blitz.remove_spaces_around_context_tags', 1);

$body = <<<BODY
{{ LITERAL }}
{{ \$var }}
{{ END }}
{{ END }}

{{ \$var }}

{{ LITERAL }}{{ BEGIN }}
{{ BEGIN }}
{{ \$var }}
{{ END }}
{{ \$var }}
BODY;

$T = new Blitz();
$T->load($body);

$params = array(
    'var' => 'variable value',
);

$T->display($params);

?>
--EXPECTF--
end with no begin (noname_loaded_from_zval: line %d, pos %d)
{{ $var }}
{{ END }}

variable value

{{ BEGIN }}
{{ BEGIN }}
{{ $var }}
variable value
