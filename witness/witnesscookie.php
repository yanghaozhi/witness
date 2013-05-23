<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
    <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
    <meta name="viewport" content="width=device-width; initial-scale=1.0; minimum-scale=1.0; maximum-scale=1.0"/> 
    <title>UC Witness染色系统</title>
</head>

<body>

     <?php
     if(empty($_POST["witness_token"]))
        echo '请输入witness的token' ;
     else
     {
        $data = $_POST["witness_token"];
        $ch = curl_init();
        curl_setopt($ch,CURLOPT_URL,"http://192.168.54.218:8099/cookie.php");
        curl_setopt($ch,CURLOPT_RETURNTRANSFER,true);
        $result = curl_exec($ch);
        $token = substr($result,0,strpos($result,':'));

        if($token == $data)
        {
            setcookie('UC_WITNESS_TOKEN', urldecode('cookie:' . $result), time() + 600);
            echo '验证成功<br/>';
            exit();
        }
        else
            echo '验证失败<br/>请输入witness的token';
     }
     ?>
     
     <form action="" method="POST">
         <input name="witness_token" value="" />
         <input type="submit" value="提交" class="ct_btn" />
     </form>

</body>
</html>
