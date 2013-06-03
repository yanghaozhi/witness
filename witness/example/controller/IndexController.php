<?php

class IndexController
{
	public function displayAction()
	{
		$userId = isset($_GET['userId'])?$_GET['userId']:"1";

		$userDetail = ServiceFactory::getService("User")->getDetail($userId);

		echo ServiceFactory::getService("Tpl")->render("index", array(
			"username"=>$userDetail['username'],
			"password"=>$userDetail['password'],
			"age"=>$userDetail['age'],
			"createTime"=>$userDetail['createTime'],
		));
	}

}



