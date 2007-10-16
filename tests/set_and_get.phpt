--TEST--
set and get
--FILE--
<?php
include('common.inc');

class Template extends Blitz
{
    var $foo;
    var $bar;

    function set($foo, $bar)
    {
        $this->foo = $foo;
        $this->bar = $bar;
        return NULL;
    }

    function get()
    {
        return $this->dump();
    }

    function dump()
    {
        debug_zval_dump($this->foo);
        debug_zval_dump($this->bar);
    }
}

$body = <<<BODY
{{set('I am foo', 'I am bar')}}
{{get()}}
BODY;

$Template = new Template();
$Template->load($body);
echo $Template->parse();

?>
--EXPECT--
string(8) "I am foo" refcount(2)
string(8) "I am bar" refcount(2)
