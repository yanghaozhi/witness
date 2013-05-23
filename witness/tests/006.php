<?php

class c1
{
    static public function a()
    {
        $v = witness_stack_info(0);
        var_dump($v);
        return new c1();
    }
};

function f1($a, $b)
{
    echo f2($a + $b, $a * $b) . "\n";
}

function f2($a, $b)
{
    $o = c1::a();
    echo ($a * $b) . "\n";
    return json_encode(array($a,$b));
}

function ff()
{
    return 1;
}

echo "abc\n";

ff();

//echo f1(5, 6) . "\n";
echo "123\n";
f1(5, 10);

echo f2(1, 3) . "\n";

//$o = c1::a();
