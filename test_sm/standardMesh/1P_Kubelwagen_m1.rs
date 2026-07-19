subshader "1P_KubelWagen_m1_Material0" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular true;
	materialDiffuse 1 1 1;
	materialSpecular 0 0 0;
	materialSpecularPower 12.5;
	texture "texture/Kubelw_Interior_I";
}

subshader "1P_KubelWagen_m1_Material1" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular true;
	materialDiffuse 1 1 1;
	materialSpecular 0 0 0;
	materialSpecularPower 12.5;
	transparent true;
	depthWrite false;
	envmap true;
	texture "texture/Kubelw_Inter_Win_I";
}

subshader "1P_KubelWagen_m1_Material2" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular true;
	materialDiffuse 1 1 1;
	materialSpecular 0 0 0;
	materialSpecularPower 12.5;
	envmap true;
	texture "texture/Kubelw_Inter_Win_I";
}

subshader "1P_KubelWagen_m1_Material4" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular true;
	materialDiffuse 1 1 1;
	materialSpecular 0 0 0;
	materialSpecularPower 12.5;
	envmap true;
	texture "texture/Kubelw_Inter_mirr_I";
}

