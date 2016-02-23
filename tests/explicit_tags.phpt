--TEST--
testing arithmetics
--FILE--
<?php
include('common.inc');
ini_set('blitz.remove_spaces_around_context_tags', 1);

$body = <<<BODY
So, if I ever want to use a tag as a literal in a template, {{var_person}} just have to double it: {{{{test}}}} will appear as {{var_test}} surrounded by 4 curly brackets!
We can go crazy {{{{{{{{{{{{{{{{ about it }}}} {{var_wicked}}
But just }}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}} will work smooth
We can even do {{{{ {{var_person}} }}}} which will result in I surrounded by 2 curly brackets on each side

BODY;

$T = new Blitz();
$T->load($body);

$T->display(
	array(
		'var_person' => 'I',
		'var_test' => 'test',
		'var_wicked' => 'Wicked!'
	)
);
?>
--EXPECT--
So, if I ever want to use a tag as a literal in a template, I just have to double it: {{test}} will appear as test surrounded by 4 curly brackets!
We can go crazy {{{{{{{{ about it }} Wicked!
But just }}}}}}}}}}}}}}}} will work smooth
We can even do {{ I }} which will result in I surrounded by 2 curly brackets on each side
