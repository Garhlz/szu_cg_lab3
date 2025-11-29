// fshader.glsl
#version 330 core

// 给光源数据一个结构体
struct Light{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;

	vec3 position;
};

// 给物体材质数据一个结构体
struct Material{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;

	float shininess;
};

in vec3 position;
in vec3 normal;

// 相机坐标
uniform vec3 eye_position;
// 光源
uniform Light light;
// 物体材质
uniform Material material;

uniform int isShadow;

out vec4 fColor;

void main()
{
	if (isShadow == 1) {
		fColor = vec4(0.0, 0.0, 0.0, 1.0);
	}
	else {

		// 将顶点坐标、光源坐标和法向量转换到相机坐标系
		// vec3 norm = (vec4(normal, 0.0)).xyz;

		// @TODO: 计算四个归一化的向量 N,V,L,R(或半角向量H)
		// N: 法向量 (需要重新归一化，因为插值会导致长度变化)
        vec3 N = normalize(normal);
        
        // V: 观察向量 (从片元指向相机)
        // eye_position 是世界坐标相机位置
        vec3 V = normalize(eye_position - position);
        
        // L: 光源向量 (从片元指向光源)
        // light.position 是世界坐标光源位置
        vec3 L = normalize(light.position - position);
        
        // R: 反射向量
        vec3 R = reflect(-L, N);



		// 环境光分量 I_a
        vec4 I_a = light.ambient * material.ambient;

        // @TODO: Task2 计算系数和漫反射分量 I_d
        float diffuse_dot = max(dot(L, N), 0.0);
        vec4 I_d = diffuse_dot * light.diffuse * material.diffuse;

        // @TODO: Task2 计算系数和镜面反射分量 I_s
        float spec_dot = max(dot(R, V), 0.0);
        float specular_dot_pow = pow(spec_dot, material.shininess);
        vec4 I_s = specular_dot_pow * light.specular * material.specular;

        // @TODO: Task2 注意如果光源在背面则去除高光
        if( dot(L, N) < 0.0 ) {
             I_s = vec4(0.0, 0.0, 0.0, 1.0);
        }

		// 合并三个分量的颜色，修正透明度
		fColor = I_a + I_d + I_s;
		fColor.a = 1.0;
		
	}
}

