class ContentsController < ApplicationController

  def index
    @contents = NotifiedContent.all(:order => "created_at desc")
  end

  def notify_image
    NotifiedContent.new({
	  ctype: 'I',
	  content: request.raw_post
	}).save
    render :nothing => true, :status => 200
  end

  def notify_sensing_data
    NotifiedContent.new({
	  ctype: 'D',
	  content: request.raw_post
	}).save
    render :nothing => true, :status => 200
  end

  def delete_all
    NotifiedContent.delete_all
	redirect_to action: 'index'
  end

end
