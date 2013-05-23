<?php

$url = 'http://u.uc.cn/abc.bz2';    //下载（mq）的url
$tmp_path = '/dev/shm/';            //临时文件目录
$db_path = '/tmp/db/';              //最终db文件存放的目录
$info_db_file = '/tmp/db/info.db';  //总揽消息db文件的路径
$try_internal = 60;                 //队列为空后等待间隔

///////////////////////////////////////////////////////////////

mkdir($tmp_path, 0777, true);
mkdir($db_path, 0777, true);

$tmp_file = tempnam($tmp_path, 'WITNESS_');
while (1)
{
    unlink($tmp_file);

    if (download($url, $tmp_file) == false)
    {
        sleep($try_internal);
        continue;
    }

    $info = parse_record($tmp_file);
    if (empty($info) === true)
    {
        continue;
    }

    insert_info($info_db_file, $info);
}

exit(0);

///////////////////////////////////////////////////////////////

function download($url, $file)
{
    $tmp = $file . '.bz2';

    $fp = fopen($tmp, "w");
    $curl = curl_init();

    curl_setopt($curl, CURLOPT_URL, $url);
    curl_setopt($curl, CURLOPT_FILE, $fp);
    curl_setopt($curl, CURLOPT_HEADER, 0);
    
    $result = curl_exec($curl);
    
    curl_close($curl);
    fclose($fp);

    if ($result == false)
    {
        return false;
    }

    $bz = bzopen($tmp, "r");
    $fp = fopen($file, "w");

    while (!feof($bz)) 
    {
        fwrite($fp, bzread($bz, 4096));
    }

    bzclose($bz);
    fclose($fp);

    return $result;
}

function parse_record($file)
{
    $db = new SQLite3($file . '.db');

    $db->exec('CREATE TABLE funcs(name TEXT UNIQUE, file TEXT, argc INT, require INT);');
    $db->exec('CREATE TABLE calls(id INTEGER PRIMARY KEY, prev INT, func TEXT, retval TEXT);');
    $db->exec('CREATE TABLE argvs(call INT, id INT, name TEXT, value TEXT, status INT);');
    $db->exec('CREATE TABLE globals(name TEXT, value TEXT);');

    $fp = fopen($file, 'r');

    $result = array();
    $line_id = 0;
    $stack = array();
    while(!feof($fp))
    {
        ++$line_id;

        $line = fgets($fp);

        $tokens = explode(' ', trim($line));
        if ($tokens[0] != '==witness==')
        {
            continue;
        }

        if (isset($tokens[1]) === false)
        {
            if (isset($func) && (isset($call)))
            {
                $cur = (empty($stack) === false) ? end($stack) : array('call'=>0);
                insert_db($db, $line_id, $cur['call'], sql_str($func['name']), sql_str($func['file']), $func['argc'], $func['require'], $call['argv']);

                array_push($stack, array('call'=>$line_id, 'func'=>$func['id'], 'name'=>$func['name']));
                //print_stack($line_id . ' push ' . $func['id'], $stack);

                unset($func);
                unset($call);
            }
            if (isset($ret))
            {
                //print_stack($line_id . ' pop ' . $ret['id'], $stack);
                $arr = array_pop($stack);
                if ($ret['id'] != $arr['func'])
                {
                    $r = $ret['id'];
                    $s = $arr['func'];
                    $n = $arr['name'];
                    echo "func $r return, but top of stack is $s - $n, not match\n";
                }
                else
                {
                    if (isset($ret['val']))
                    {
                        $val = sql_str($ret['val']);
                        $id = $arr['call'];
                        $sql = "UPDATE calls SET retval = $val WHERE id = $id;";
                        //echo $sql . "\n";
                        $db->exec($sql);
                    }
                }
                unset($ret);
            }
            continue;
        }

        switch ($tokens[1])
        {
        case 'global':
            $n = sql_str($tokens[2]);
            $v = sql_str($tokens[4]);
            $sql = "INSERT INTO globals VALUES ($n, $v);";
            //echo $sql . "\n";
            $db->exec($sql);
            $result[$n] = $v;
            break;
        case 'file':
            $func['file'] = (isset($tokens[3]) === true) ? $tokens[3] : '';
            break;
        case 'func':
            $func['id'] = $tokens[3];
            $func['name'] = $tokens[4];
            break;
        case 'prev':
            //$call['prev'] = $tokens[3];
            //if ($tokens[3] != end($stack_func))
            //{
            //    $name = end($stack_func);
            //    $n = $func['name'];
            //    echo "func $n has prev $tokens[3], but stack name is $name\n";
            //}
            break;
        case 'argc':
            $func['argc'] = intval($tokens[4]);
            $func['require'] = intval($tokens[6]);
            $call['argv'] = array();
            break;
        case 'argv':
            $call['argv'][$tokens[4]] = $tokens[5];
            break;
        case 'ret':
            $ret['id'] = $tokens[3];
            break;
        case 'retval':
            $ret['val'] = (isset($tokens[3]) === true) ? $tokens[3] : '';
            break;
        case 'START':
            break;
        case 'END':
            echo "PARSE END\n";
            break;
        case 'WARN':
            $call['argv']['unknown'] = $tokens[7];
            break;
        case 'token':
            echo "token - " . $tokens[3] . "\n";
            $result['token'] = $tokens[3];
            break;
        case 'date':
            $result['date'] = $tokens[3];
            break;
        default:
            echo $line;
            break;
        }
    }

    fclose($fp);

    $db->close();

    return $result;
}

function insert_info($info_db, $info)
{
    $db = new SQLite3($info_db);

    $token   = sql_str(isset($info['token']) ? $info['token'] : null);
    $date    = sql_str(isset($info['date']) ? $info['date'] : null);
    $post    = sql_str(isset($info['_POST']) ? $info['_POST'] : null);
    $get     = sql_str(isset($info['_GET']) ? $info['_GET'] : null);
    $session = sql_str(isset($info['_SESSION']) ? $info['_SESSION'] : null);
    $env     = sql_str(isset($info['_ENV']) ? $info['_ENV'] : null);
    $cookie  = sql_str(isset($info['_COOKIE']) ? $info['_COOKIE'] : null);
    $server  = sql_str(isset($info['_SERVER']) ? $info['_SERVER'] : null);

    $sql = "INSERT INTO operation VALUES ($token, $date, $post, $get, $session, $env, $cookie, $server);";
    //echo $sql . "\n";
    $db->exec($sql);

    $db->close();
}

function insert_db($db, $id, $prev, $func, $file, $total, $require, $args)
{
    $sql = "SELECT * FROM funcs WHERE name=$func;";
    //echo $sql . "\n";
    $row = $db->query($sql)->fetchArray();
    if ($row == null)
    {
        $sql = "INSERT INTO funcs VALUES ($func, $file, $total, $require);";
        //echo $sql . "\n";
        $db->exec($sql);
    }

    $sql = "INSERT INTO calls VALUES ($id, $prev, $func, '');";
    //echo $sql . "\n";
    $db->exec($sql);

    $i = 0;
    foreach ($args as $k=>$v)
    {
        $status = ($i < $require) ? 0 : ($i < $total) ? 1 : 2;
        $k = sql_str($k);
        $v = sql_str($v);
        $sql = "INSERT INTO argvs VALUES ($id, $i, $k, $v, $status);";
        //echo $sql . "\n";
        $db->exec($sql);

        ++$i;
    }
}

function print_stack($header, $stack)
{
    echo $header . "\n";
    foreach ($stack as $func)
    {
        echo $func['name'] . ' - ' . $func['call'] . ' - ' . $func['func'] . "\n";
    }
    echo "\n";
}

function sql_str($str)
{
    return "'" . str_replace("'", "''", $str) . "'";
}
