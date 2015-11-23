--TEST--
include scope
--FILE--
<?php
include('common.inc');
ini_set('blitz.remove_spaces_around_context_tags', 1);
ini_set('blitz.warn_context_duplicates', 0);

$body = <<<BODY
Global top = "{{ serialize(\$_top) }}"

{{ BEGIN arr }}
{{ include('include-scope-inner.tpl') }}
{{ END arr }}

Specific scoped loop:

{{ BEGIN arr }}
{{ include('include-scope-inner.tpl',   scope  =    \$scope ,   a = \$scope.a   ) }}
{{ END arr }}

Current scoped loop:

{{ BEGIN arr }}
{{ include('include-scope-inner.tpl',  scope   =   \$_ , a = \$_top.a   ) }}
{{ END arr }}

BODY;

$T = new Blitz();
$T->load($body);

$T->display(
	array(
		'arr' => array(
			array('a' => '1-a', 'b' => '1-b'),
			array('a' => '2-a', 'b' => '2-b')
		),
		'scope' => array(
			'a' => 'scope-a'
		),
		'a' => 'top-a'
	)
);

?>
--EXPECT--
Global top = "a:3:{s:3:"arr";a:2:{i:0;a:2:{s:1:"a";s:3:"1-a";s:1:"b";s:3:"1-b";}i:1;a:2:{s:1:"a";s:3:"2-a";s:1:"b";s:3:"2-b";}}s:5:"scope";a:1:{s:1:"a";s:7:"scope-a";}s:1:"a";s:5:"top-a";}"

a = 1-a, b = 1-b
top = "a:2:{s:1:"a";s:3:"1-a";s:1:"b";s:3:"1-b";}"
current = "a:2:{s:1:"a";s:3:"1-a";s:1:"b";s:3:"1-b";}"

a = 2-a, b = 2-b
top = "a:2:{s:1:"a";s:3:"2-a";s:1:"b";s:3:"2-b";}"
current = "a:2:{s:1:"a";s:3:"2-a";s:1:"b";s:3:"2-b";}"


Specific scoped loop:

a = scope-a, b = 1-b
top = "a:3:{s:5:"scope";a:1:{s:1:"a";s:7:"scope-a";}s:1:"a";s:7:"scope-a";s:1:"b";s:3:"1-b";}"
current = "a:3:{s:5:"scope";a:1:{s:1:"a";s:7:"scope-a";}s:1:"a";s:7:"scope-a";s:1:"b";s:3:"1-b";}"

a = scope-a, b = 2-b
top = "a:3:{s:5:"scope";a:1:{s:1:"a";s:7:"scope-a";}s:1:"a";s:7:"scope-a";s:1:"b";s:3:"2-b";}"
current = "a:3:{s:5:"scope";a:1:{s:1:"a";s:7:"scope-a";}s:1:"a";s:7:"scope-a";s:1:"b";s:3:"2-b";}"


Current scoped loop:

a = top-a, b = 1-b
top = "a:3:{s:5:"scope";a:2:{s:1:"a";s:3:"1-a";s:1:"b";s:3:"1-b";}s:1:"a";s:5:"top-a";s:1:"b";s:3:"1-b";}"
current = "a:3:{s:5:"scope";a:2:{s:1:"a";s:3:"1-a";s:1:"b";s:3:"1-b";}s:1:"a";s:5:"top-a";s:1:"b";s:3:"1-b";}"

a = top-a, b = 2-b
top = "a:3:{s:5:"scope";a:2:{s:1:"a";s:3:"2-a";s:1:"b";s:3:"2-b";}s:1:"a";s:5:"top-a";s:1:"b";s:3:"2-b";}"
current = "a:3:{s:5:"scope";a:2:{s:1:"a";s:3:"2-a";s:1:"b";s:3:"2-b";}s:1:"a";s:5:"top-a";s:1:"b";s:3:"2-b";}"
