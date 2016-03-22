--TEST--
auto-escaping setting per template.
--FILE--
<?php
include('common.inc');

ini_set('default_charset', 'UTF-8');

$data = array(
    'a' => "this is 'a' \"test\"",
    'b' => '<a href="null">',
    'c' => '<>@#$%^&*()_=\'"',
);

ini_set('blitz.auto_escape', 0);

$T = new Blitz();
$T->load("{{ \$a }} {{ \$b }} {{ \$c }}");

$T->display($data);
$T->clean();
echo "\n";

echo var_dump($T->setAutoEscape(true));
$T->display($data);
$T->clean();
echo "\n";

echo var_dump($T->setAutoEscape(false));
$T->display($data);
$T->clean();
echo "\n";

unset($T);

ini_set('blitz.auto_escape', 1);

$T = new Blitz();
$T->load("{{ \$a }} {{ \$b }} {{ \$c }}");

$T->display($data);
$T->clean();
echo "\n";

echo var_dump($T->setAutoEscape(false));
$T->display($data);
$T->clean();
echo "\n";

echo var_dump($T->setAutoEscape(true));
$T->display($data);
$T->clean();

unset($T);

?>
--EXPECT--
this is 'a' "test" <a href="null"> <>@#$%^&*()_='"
bool(false)
this is &apos;a&apos; &quot;test&quot; &lt;a href=&quot;null&quot;&gt; &lt;&gt;@#$%^&amp;*()_=&apos;&quot;
bool(true)
this is 'a' "test" <a href="null"> <>@#$%^&*()_='"
this is &apos;a&apos; &quot;test&quot; &lt;a href=&quot;null&quot;&gt; &lt;&gt;@#$%^&amp;*()_=&apos;&quot;
bool(true)
this is 'a' "test" <a href="null"> <>@#$%^&*()_='"
bool(false)
this is &apos;a&apos; &quot;test&quot; &lt;a href=&quot;null&quot;&gt; &lt;&gt;@#$%^&amp;*()_=&apos;&quot;
