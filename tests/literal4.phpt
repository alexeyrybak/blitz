--TEST--
literal #4
--FILE--
<?php
include('common.inc');
ini_set('blitz.remove_spaces_around_context_tags', 1);

$body = <<<BODY
{{ LITERAL }}
{{ \$var }}
{{ END e1 }}
{{ END e2 }}

{{ \$var }}

{{ LITERAL }}{{ BEGIN }}
{{ BEGIN }}
{{ END e3 }}
{{ \$var }}
{{ END e4 }}{{ END e5 }}
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
end with no begin (noname_loaded_from_zval: line %d, pos %d)
end with no begin (noname_loaded_from_zval: line %d, pos %d)
{{ $var }}
{{ END e2 }}

variable value

{{ BEGIN }}
{{ BEGIN }}
variable value
{{ END e4 }}{{ END e5 }}
variable value
