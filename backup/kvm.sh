#!/bin/bash 

dst_dir=/var/lib/libvirt/images/
img_dir=/var/lib/libvirt/images/
xml_dir=/etc/libvirt/qemu/
src_img=vmx.qcow2 
src_vm_name=vmx  
src_xml=vmx.xml
red=\033[31m
green=\033[32m
yellow=\033[33m
normal=\033[0m

echo $dst_vm_name  
echo $mac  
echo $vmuid  
virsh  list  --all  

  
create_vm() {
        echo "$yellow Copy $org_image  to $dst_dir/base.qcow2 ... $normal"
        if [ ! -f $dest_dir/base.qcow2 ]
        then
            cp $org_image  $dst_dir/base.qcow2
        else
            rm -f $dst_dir/base.qcow2
            cp $org_image  $dst_dir/base.qcow2
        fi
        echo "$green Base image copy Done $normal"
	
        #用src_img基础镜像来创建增量镜像  
        #-b指定后备镜像(即基础镜像)，-f指定格式。只有格式为qcow2才能实现后备镜像和增量镜像功能。  
        mac="00:"`echo $RANDOM | md5sum | sed 's/../&:/g' | cut -c1-14`  
        vmuid=$(uuidgen)  
        #dst_vm_name=$src_vm_name$1
        #dst_img=$src_vm_name$1.qcow2
        #dst_xml=$src_vm_name$1.xml
        
        pushd $img_dir
        qemu-img  create  -b ${dst_dir}base.qcow2 -f qcow2 $dst_dir/testvm$1.qcow2 & >/dev/null
        sleep 1
        chmod  -v  444  base.qcow2
        qemu-img  info  base.qcow2 
        qemu-img  info   testvm$1.qcow2
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


run_vm() {
        #根据虚拟机的xml模板文件创建新虚拟机  
        virsh define /etc/libvirt/qemu/${dst_vm_name}.xml  
	sleep 1
        virsh  start   ${dst_vm_name}  
        # virsh  autostart   ${dst_vm_name}  
        virsh  list  --all  
        ls $img_dir -l 
            #功能说明  
            #virsh  start   ${dst_vm_name}   --console    #启动虚拟机并进入console控制台  
            #virsh  console  ${dst_vm_name}    #按ctl+]退出console控制台  
            #virt-viewer   ${dst_vm_name}  
              
            #read  -p  "按回车键进入${dst_vm_name}的console控制台,进入console后按ctrl+]退出..."  
            #virsh  console  ${dst_vm_name}
        
}

shutdown_vm(){
	sudo virsh shutdown vmx$i
}

remove_vm() {
	sudo virsh destroy vmx$i 2> /dev/null
	sudo virsh undefine vmx$i
	sudo virsh pool-refresh default
	sudo virsh vol-delete --pool default vmx$i.qcow2
}


USAGE=$(cat <<- EOF
    Version : 0.0.1
    This script is used to vdi kvm test

    Usage: 
      ./kvm.sh -c start_vm_index vm_count original_qcow2_name    # ./kvm.sh -c 从第几号开始创建vm 要创建的vm数量 原始的qcow2镜像名称
      ./kvm.sh -r start_vm_index vm_count
      ./kvm.sh -cr start_vm_index vm_count original_qcow2_name    # 创建并运行vm 
      ./kvm.sh -s start_vm_index vm_count
      ./kvm.sh -d start_vm_index vm_count

    Options:
      -c or --create    create increment images VMs based on source image. 
      -r or --run       run the VMs
      -s or --stop      shutdown VMs 
      -d or --del       delete qcow2 images and xml config. 
      -h or --help      help.

    Example:
      ./kvm.sh -c 1 50 /var/lib/libvirt/images/original_xxx.qcow2 
      ./kvm.sh -r 1 50
      ./kvm.sh -cr 1 50 /var/lib/libvirt/images/original_xxx.qcow2 
      ./kvm.sh -s 1 50
      ./kvm.sh -d 1 50

EOF
)

__CREATE=
__RUN=
__STOP=
__DELETE=

while [ "$#" -gt 0  ];do
  case "$1" in
    -cr | --create_run)
        __CREATE="true"
        __RUN="true"
        shift
        if [ "$#" -lt 4 ]; then
            echo "$USAGE"
            echo
            echo "$red [ERROR] -cr or --create_run need more paras. $normal"
            echo
            exit -1
        fi
        ;;
    -c | --create)
        __CREATE="true"
        shift
        if [ "$#" -lt 4 ]; then
            echo "$USAGE"
            echo
            echo "$red [ERROR] -c or --create need more paras. $normal"
            echo
            exit -1
        fi
        ;;
    -r | --run)   
        shift 
        if [ "$#" -lt 3 ]; then
            echo "$USAGE"
            echo
            echo "$red [ERROR] -r or --run need more paras. $normal"
            echo
            exit -1
        fi
        __RUN="true"
        shift
        ;;
    -s | --stop)
        shift
        if [ "$#" -lt 3 ]; then
            echo "$USAGE"
            echo
            echo "$red [ERROR] -s or --stop need more paras. $normal"
            echo
            exit -1
        fi
        __STOP="true"
	shift
        ;;
    -d | --del)
        __DELETE="true"
        if [ "$#" -lt 3 ]; then
            echo "$USAGE"
            echo
            echo "$red [ERROR] -d or --del need more paras. $normal"
            echo
            exit -1
        fi
        shift
        ;;
    -h | --help)
        echo "$USAGE"
        exit
        ;;
    *)
        echo "$USAGE"
        echo "$red [ERROR] Invalid parameter $1 $normal"
        exit -1
        ;;
  esac
done


if [[ $__CREATE = "true" ]] && [[ $__RUN = "true" ]]
then
    start_vm_index=$2
    vm_count=$3
    org_image=$4

    if (( $start_vm_index < 0 )) || (( $start_vm_index > 254 ))
    then
        echo "$red start_vm_index should between 0~254 $normal"
        exit
    fi

    if (( $vm_count < 1 )) || (( $vm_count > 254 ))
    then
        echo "$red vmcount should between 1~254 $normal"
        exit
    fi

    if [ ! -f $org_image ]
    then
       echo "$red File $org_image not found! $normal" >&2
       exit
    fi
    
    if [ ! -d $dst_dir ]
    then
       echo "$red Dir $dst_dir not found! $normal" >&2
       exit
    fi

    for (( i = $start_vm_index; i <= (($start_vm_index+$vm_count)); i++)); do
	echo "$yellow Create increment file $dest_dir/testvm${i}.qcow2... $normal"
	if [ ! -f $dst_dir/testvm${i}.qcow2 ]
	then
		create_vm $i
	else
		echo "Increment file $dst_dir/testvm${i}.qcow2 exist"
	fi
	echo "$green Done."
    	sleep 1
	run_vm
	sleep 5
    
    done
    
elif [[ $__CREATE = "true" ]] && [[ $__RUN != "true" ]]
then
	create_vm

elif [[ $__CREATE != "true" ]] && [[ $__RUN = "true" ]]
then
	run_vm

elif [[ $__STOP = "true" ]] 
then
	shutdown_vm
	
elif [[ $__DELETE= "true" ]] 
then
	remove_vm
fi



if [[ -z $__CREATE]] && [[ -z $__RUN]] && [[ -z $__STOP]] && [[ -z $__DELETE]]
then
    echo "$USAGE"
fi


exit
