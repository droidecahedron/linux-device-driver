#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(.gnu.linkonce.this_module) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section(__versions) = {
	{ 0x9de7765d, "module_layout" },
	{ 0x69de56ed, "param_ops_int" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x2142178f, "cdev_del" },
	{ 0x37a0cba, "kfree" },
	{ 0xc5850110, "printk" },
	{ 0x64fe0dcf, "device_create" },
	{ 0x18f0dc72, "cdev_init" },
	{ 0x19c48f0b, "cdev_add" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0xeb233a45, "__kmalloc" },
	{ 0x296aaca8, "__class_create" },
	{ 0x3fd78f3b, "register_chrdev_region" },
	{ 0x362ef408, "_copy_from_user" },
	{ 0xcf2a6966, "up" },
	{ 0xb44ad4b3, "_copy_to_user" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0x81b395b3, "down_interruptible" },
	{ 0xfb578fc5, "memset" },
	{ 0xb1e12d81, "krealloc" },
	{ 0xbdfb6dbb, "__fentry__" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "ACF3D2AEA685710209EE13C");
