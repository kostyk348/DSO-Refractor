shader "Material4"
{
	technique
	{
		pass
		{
			lighting true;
			lightingSpecular true;
			materialAmbient 1 1 1;
			materialDiffuse 1 1 1;
			materialSpecular 0 0 0;
			materialSpecularPower 0.5;

			stage
			{
				texture "texture/sherBO_f";
				combine color mul texture diffuse;
				combine alpha single texture;
			}
		}
	}
}

shader "Material5"
{
	technique
	{
		pass
		{
			lighting true;
			lightingSpecular true;
			materialAmbient 1 1 1;
			materialDiffuse 1 1 1;
			materialSpecular 0 0 0;
			materialSpecularPower 0.5;

			stage
			{
				texture "texture/sherBI_f";
				combine color mul texture diffuse;
				combine alpha single texture;
			}
		}
	}
}

shader "Material6"
{
	technique
	{
		transparent true;
		pass
		{
			cullMode none;
			lighting true;
			lightingSpecular true;
			materialAmbient 1 1 1;
			materialDiffuse 1 1 1;
			materialSpecular 0 0 0;
			materialSpecularPower 0.5;
			alphaTest greater 0.7;


			stage
			{
				texture "texture/sherBt_f";
				combine color mul texture diffuse;
				combine alpha single texture;
			}
		}
	}
}

