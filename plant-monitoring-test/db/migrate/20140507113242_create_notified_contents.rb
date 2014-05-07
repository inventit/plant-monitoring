class CreateNotifiedContents < ActiveRecord::Migration
  def change
    create_table :notified_contents do |t|
      t.string :ctype
	  t.text :content
      t.timestamps
    end
  end
end
