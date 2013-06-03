<?php
class User
{
	public function getDetail($userId)
	{
		$data = DaoFactory::getDao("User")->getDetailByUid($userId);
		$data['createTime'] = date("Y-m-d H:i:s", $data['createTime']);
		return $data;
	}
}

