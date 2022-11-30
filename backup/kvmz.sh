#!/bin/bash 

img_dir=/var/lib/libvirt/images/
xml_dir=/etc/libvirt/qemu/
src_img=vmx.qcow2 
src_vm_name=vmx  
src_xml=vmx.xml
# vport=5915  


if [ $# -lt 1 ]
then
    echo -e "\e[33mUsage: $0 vm_numbers \e[0m"
    exit
fi

echo $dst_vm_name  
echo $mac  
echo $vmuid  
virsh  list  --all  

#提示用户输入信息  
# read  -p  '请输入虚拟机的模板机名称，例:vmx0:'   src_vm_name  
# read  -p  '请输入新虚拟机的名称，例:vmx1:'   dst_vm_name  
# read  -p  '请输入新虚拟的vnc端口，例:5912:'   vport  
  
kvm_clone() {
#用src_img基础镜像来创建增量镜像  
#-b指定后备镜像(即基础镜像)，-f指定格式。只有格式为qcow2才能实现后备镜像和增量镜像功能。  
mac="00:"`echo $RANDOM | md5sum | sed 's/../&:/g' | cut -c1-14`  
vmuid=$(uuidgen)  
dst_vm_name=$src_vm_name$1
dst_img=$src_vm_name$1.qcow2
dst_xml=$src_vm_name$1.xml

pushd $img_dir
qemu-img  create  -b ${img_dir}${src_img} -f qcow2 $img_dir$dst_img & >/dev/null
sleep 1
chmod  -v  444  $src_img
qemu-img  info  $src_img 
qemu-img  info   $dst_img
popd
  
#编辑新虚拟机的xml配置文件  
pushd $xml_dir
cp  -pv  ${src_vm_name}.xml  ${dst_vm_name}.xml  
sed  -i   "/uuid/c<uuid>$vmuid</uuid>"   ${dst_vm_name}.xml  
sed  -i   "s/${src_vm_name}/${dst_vm_name}/"   ${dst_vm_name}.xml  
sed  -i   "/mac address/c<mac address='$mac'/>"   ${dst_vm_name}.xml  
# sed  -i   "s/5910/${vport}/"  ${dst_vm_name}.xml  
grep  $vmuid   ${dst_vm_name}.xml  
grep  $mac    ${dst_vm_name}.xml  
grep  $dst_vm_name    ${dst_vm_name}.xml  
#grep  $vport    ${dst_vm_name}.xml  
popd
}



for (( i = 1; i <= $1; i++)); do
	kvm_clone $i
	sleep 1

        #根据虚拟机的xml模板文件创建新虚拟机  
        virsh define /etc/libvirt/qemu/${dst_vm_name}.xml  
	sleep 1
        virsh  start   ${dst_vm_name}  
        # virsh  autostart   ${dst_vm_name}  
        virsh  list  --all  
        ls $img_dir -l 
        sleep 5
          
        #功能说明  
        #virsh  start   ${dst_vm_name}   --console    #启动虚拟机并进入console控制台  
        #virsh  console  ${dst_vm_name}    #按ctl+]退出console控制台  
        #virt-viewer   ${dst_vm_name}  
          
        #read  -p  "按回车键进入${dst_vm_name}的console控制台,进入console后按ctrl+]退出..."  
        #virsh  console  ${dst_vm_name}
done

