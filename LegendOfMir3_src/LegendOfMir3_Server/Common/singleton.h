#ifndef SINGLETON_H
#define SINGLETON_H

// ==================Ascent中的模板单件法===================
// 个人感觉缺点:构造函数不为私有函数的缺点：暴露这个类，多次构造会出现assert( s_pSingleton == 0 );
// 用法优点：
//    1.继承减少代码量 
//    2.构造函数不为私有的优点：初始化可以多样，
//         可急型(定义全局变量createFileSingleton，不用管生存周期)
//         可缓型(初始化变量initialiseSingleton后进行new构造和delete析构
//                特点,只能构造一个对象,  构造第二次new就出现assert( s_pSingleton == 0 ))

// 缓型宏后要new 和 delete对象
#define InitialiseSingleton( type ) \
	template <> type * CSingleton < type > :: s_pSingleton = 0

// 急型宏后不用管该对象的初始化和释放
#define CreateSingleton( type ) \
	InitialiseSingleton( type ); \
	type the##type


template<class T>
class  CSingleton
{
public:
	CSingleton( ) {
		//assert( s_pSingleton == NULL );
		s_pSingleton = static_cast<T*>(this);
	}
	virtual ~CSingleton( ) {	s_pSingleton = 0;	}

	static T & GetInstance()		{ /*assert( s_pSingleton );*/ return *s_pSingleton; }
	static T * GetInstancePtr( )	{ return s_pSingleton; }

protected:
	static T * s_pSingleton;
};

#endif